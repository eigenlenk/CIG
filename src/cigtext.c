#include "cigtext.h"
#include "utf8.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define WORD_SPACING 8
#define MAX_TAG_NAME_LEN 16
#define MAX_TAG_VALUE_LEN 32

typedef struct {
  utf8_string slice;
  struct { unsigned short w, h; } bounds;
  cig_font_ref font;
  cig_text_color_ref color;
  enum {
    NO_TRAILING_SPACING = CIG_BIT(0)
  } flags;
} span_t;

typedef struct {
  cig_id_t hash;
  span_t spans[CIG_LABEL_SPANS_MAX];
  size_t span_count;
} label_t;

typedef struct {
  bool open,
       terminating,
       _naming;
  char name[MAX_TAG_NAME_LEN],
       value[MAX_TAG_VALUE_LEN];
} tag_parser_t;

static cig_text_render_callback_t render_callback = NULL;
static cig_text_measure_callback_t measure_callback = NULL;
static cig_font_query_callback_t font_query = NULL;
static cig_font_ref default_font = NULL;

static void render_spans(span_t*, size_t, cig_text_properties_t*);
static bool parse_tag(tag_parser_t*, utf8_char, uint32_t);
static void* get_va_arg(va_list*, size_t);

void cig_set_text_render_callback(cig_text_render_callback_t callback) {
  render_callback = callback;
}

void cig_set_text_measure_callback(cig_text_measure_callback_t callback) {
  measure_callback = callback;
}

void cig_set_font_query_callback(cig_font_query_callback_t callback) {
  font_query = callback;
}

void cig_set_default_font(cig_font_ref font_ref) {
  default_font = font_ref;
}

void cig_label(cig_text_properties_t props, const char *text, ...) {
  props.font = props.font ? props.font : default_font;
    
  cig_id_t hash = cig_hash(text);
  tag_parser_t tag_parser = { 0 };
  
  label_t *label = CIG_ALLOCATE(label_t);

  if (label->hash != hash) {
    label->hash = hash;
    label->span_count = 0;
    
    utf8_string utext = make_utf8_string(text);
    utf8_char_iter iter = make_utf8_char_iter(utext);
    
    bool tag_stage_changed;
    utf8_char ch;
    uint32_t cp;
    size_t span_start = 0, cur = 0;
    
 		va_list args;
		va_start(args, text);
    
    struct { size_t count; cig_font_ref fonts[4]; } font_stack = { 0 };
    struct { size_t count; cig_text_color_ref colors[4]; } color_stack = { 0 };
    
    while ((ch = next_utf8_char(&iter)).byte_len > 0 && (cp = unicode_code_point(ch))) {
      /*printf("character: %.*s\t", (int)ch.byte_len, ch.str);
      printf("unicode code point: U+%04X\t", cp);
      printf("open = %d\t", tag_parser.open);
      printf("span_start = %d\n", (int)span_start);*/
      
      /* Tag was either opened or closed */
      if ((tag_stage_changed = parse_tag(&tag_parser, ch, cp))) {
        /* Tag closed */
        if (!tag_parser.open) {
          span_start = cur + ch.byte_len;

          if (!strcasecmp(tag_parser.name, "font")) {
            if (tag_parser.terminating) {
              font_stack.count--;
            } else {
              font_stack.fonts[font_stack.count++] = get_va_arg(&args, atoi(tag_parser.value));
            }
          } else if (!strcasecmp(tag_parser.name, "color")) {
            if (tag_parser.terminating) {
              color_stack.count--;
            } else {
              color_stack.colors[color_stack.count++] = get_va_arg(&args, atoi(tag_parser.value));
            }
          }
        }
      } else if (tag_parser.open) {
        cur += ch.byte_len;
        continue;
      }
      
      /* Span terminated: a) basic space character, b) tag encountered or c) end of string */
      if ((cur > span_start && (cp == 0x20 || tag_stage_changed)) || (cur + ch.byte_len == utext.byte_len)) {
        if (cur + ch.byte_len == utext.byte_len) {
          cur += ch.byte_len;
        }
        utf8_string slice = slice_utf8_string(utext, span_start, cur - span_start);
        cig_vec2_t bounds = measure_callback
          ? measure_callback(slice.str, slice.byte_len, font_stack.count > 0 ? font_stack.fonts[font_stack.count-1] : props.font)
          : cig_vec2_zero();
        
        label->spans[label->span_count++] = (span_t) { 
          .slice = slice,
          .bounds = { bounds.x, bounds.y },
          .font = font_stack.count > 0 ? font_stack.fonts[font_stack.count-1] : NULL,
          .color = color_stack.count > 0 ? color_stack.colors[color_stack.count-1] : NULL,
          .flags = cp != 0x20 ? NO_TRAILING_SPACING : 0
        };
        
        // printf("appended slice %d\n", label->span_count-1);
        
        span_start = cur + ch.byte_len;
      }
      
      cur += ch.byte_len;
    }
    
    va_end(args);
  }
  
  if (render_callback) {
    render_spans(&label->spans[0], label->span_count, &props);
  }
}

/* ╔════════════════════════════════════════════╗
   ║            INTERNAL FUNCTIONS              ║
   ╚════════════════════════════════════════════╝ */

static void render_spans(span_t *first, size_t count, cig_text_properties_t *props) {
  register const cig_rect_t absolute_rect = cig_rect_inset(cig_absolute_rect(), cig_frame()->insets);
  register int lines, x, y, w, dx, dy;
  register span_t *span, *line_start, *line_end, *last = first + count;
  register const cig_font_info_t font_info = font_query(props->font);
  
  static double alignment_constant[3] = { 0, 0.5, 1 };
  
  /* Figure out number of lines needed */
  for (span = first, x = 0, y = 0; span < last; span++) {
    if (x + span->bounds.w > absolute_rect.w && x > 0) {
      x = 0;
      y ++;
    }
    
    x += span->bounds.w + (span->flags & NO_TRAILING_SPACING ? 0 : WORD_SPACING);
  }
  
  lines = 1 + y;
  dy = absolute_rect.y + (int)(absolute_rect.h - (lines * font_info.height)) * alignment_constant[props->alignment.vertical];
  
  for (line_start = span = first, x = 0; span < last;) {
    if (x + span->bounds.w > absolute_rect.w) {
      if (!x) {
        w = span->bounds.w;
        line_end = span+1;
      } else {
        line_end = span;
        w = x - WORD_SPACING;
        x = 0;
      }
    } else if (span == last-1) {
      line_end = last;
      w = x + span->bounds.w;
    } else {
      line_end = NULL;
      x += span->bounds.w + (span->flags & NO_TRAILING_SPACING ? 0 : WORD_SPACING);
    }
    
    if (line_end) {
      dx = dx = absolute_rect.x + (int)((absolute_rect.w - w) * alignment_constant[props->alignment.horizontal]);
      
      for (span = line_start; span < line_end; dx += (span->flags & NO_TRAILING_SPACING ? 0 : WORD_SPACING) + (span++)->bounds.w) {
        cig_font_info_t span_font_info = span->font ? font_query(span->font) : font_info;
        render_callback(
          cig_rect_make(
            dx,
            dy + (span->font ? ((font_info.height+font_info.baseline_offset)-(span_font_info.height+span_font_info.baseline_offset)) : 0),
            span->bounds.w,
            span->bounds.h
          ),
          span->slice.str,
          span->slice.byte_len,
          span->font ? span->font : props->font,
          span->color ? span->color : props->color
        );
      }

      span = line_start = line_end;
      dy += font_info.height;
      continue;
    }
    
    span++;
  }
}

static bool parse_tag(tag_parser_t *this, utf8_char ch, uint32_t cp) {
  if (!this->open) {
    if (cp == 0x3C) { /* < */
      this->open = true;
      this->terminating = false;
      this->_naming = true;
      strcpy(this->name, "");
      strcpy(this->value, "");
      printf("open <\n");
      return true;
    }
  } else {
    if (cp == 0x2F) { /* / */
      this->terminating = true;
      printf(" terminating /\n");
    } else if (cp == 0x3E) { /* > */
      this->open = false;
      printf("close > (%s = %d)\n", this->name, this->value);
      return true;
    } else if (cp == 0x3D && this->_naming) { /* = */
      this->_naming = false; /* Switch to collecting tag value */
    } else if (cp == 0x20) {
      /* Ignore space */
    } else {
      if (this->_naming) {
        snprintf(this->name+strlen(this->name), MAX_TAG_NAME_LEN-strlen(this->name), "%.*s", (int)ch.byte_len, ch.str);
      } else {
        snprintf(this->value+strlen(this->value), MAX_TAG_VALUE_LEN-strlen(this->value), "%.*s", (int)ch.byte_len, ch.str);
      }
    }
  }
  
  return false;
}

static void* get_va_arg(va_list *list, size_t position) {
  register size_t i;
  void *arg;
  va_list copy;
  va_copy(copy, *list);
  for (i = 0; i < position; ++i) { va_arg(copy, void*); }
  arg = va_arg(copy, void*);
  va_end(copy);
  return arg;
}














