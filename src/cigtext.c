#include "cigtext.h"
#include "utf8.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define MAX_TAG_NAME_LEN 16
#define MAX_TAG_VALUE_LEN 32

typedef struct {
  const char *str;                        // 4
  cig_font_ref font;                      // 4
  cig_text_color_ref color;               // 4
  struct { unsigned short w, h; } bounds; // 4
  unsigned char byte_len;                 // 1
  unsigned char spacing_after;            // 1
  unsigned char style_flags;              // 1
} span_t;                                 // 20 bytes total

typedef struct {
  cig_id_t hash;                          // 4
  size_t span_count;                      // 4  
  span_t spans[CIG_LABEL_SPANS_MAX];      // 320
} label_t;                                // 328 bytes total

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
static char printf_buf[CIG_LABEL_PRINTF_BUF_LENGTH];

static void render_spans(span_t*, size_t, cig_text_properties_t*);
static bool parse_tag(tag_parser_t*, utf8_char, uint32_t);

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
  if (!props.font) {
    props.font = default_font;
  }
  
  if (props.alignment.horizontal == CIG_TEXT_ALIGN_DEFAULT) {
    props.alignment.horizontal = CIG_TEXT_ALIGN_CENTER;
  }
  
  if (props.alignment.vertical == CIG_TEXT_ALIGN_DEFAULT) {
    props.alignment.vertical = CIG_TEXT_ALIGN_MIDDLE;
  }
  
  char *str;
  
  if (props.flags & CIG_TEXT_FORMATTED) {
    va_list args;
    va_start(args, text);
    vsprintf(printf_buf, text, args);
    va_end(args);
    str = printf_buf;
  } else {
    str = (char *)text;
  }
  
  cig_id_t hash = cig_hash(str);
  tag_parser_t tag_parser = { 0 };
  
  label_t *label = CIG_ALLOCATE(label_t);
  
  if (label->hash != hash) {
    label->hash = hash;
    label->span_count = 0;
    
    utf8_string utext = make_utf8_string(str);
    utf8_char_iter iter = make_utf8_char_iter(utext);
    
    bool tag_stage_changed;
    utf8_char ch;
    uint32_t cp;
    size_t span_start = 0, cur = 0;
    cig_font_ref font;
    cig_font_info_t font_info;
    cig_text_style_t style = props.style;
    
    struct { size_t count; cig_font_ref fonts[4]; } font_stack = { 0 };
    struct { size_t count; cig_text_color_ref colors[4]; } color_stack = { 0 };
    
    while ((ch = next_utf8_char(&iter)).byte_len > 0 && (cp = unicode_code_point(ch))) {
      /*printf("character: %.*s\t", (int)ch.byte_len, ch.str);
      printf("unicode code point: U+%04X\t", cp);
      printf("open = %d\t", tag_parser.open);
      printf("span_start = %d\n", (int)span_start);*/
      
      /* Tag was either opened or closed */
      if (props.flags & CIG_TEXT_FORMATTED && (tag_stage_changed = parse_tag(&tag_parser, ch, cp))) {
        /* Tag closed */
        if (!tag_parser.open) {
          span_start = cur + ch.byte_len;

          if (!strcasecmp(tag_parser.name, "font")) {
            if (tag_parser.terminating) {
              font_stack.count--;
            } else {
              font_stack.fonts[font_stack.count++] = (cig_font_ref)strtol(tag_parser.value, NULL, 16);
            }
          } else if (!strcasecmp(tag_parser.name, "color")) {
            if (tag_parser.terminating) {
              color_stack.count--;
            } else {
              color_stack.colors[color_stack.count++] = (cig_text_color_ref)strtol(tag_parser.value, NULL, 16);
            }
          } else if (!strcasecmp(tag_parser.name, "b")) {
            if (tag_parser.terminating) {
              style &= ~CIG_TEXT_STYLE_BOLD;
            } else {
              style |= CIG_TEXT_STYLE_BOLD;
            }
          } else if (!strcasecmp(tag_parser.name, "i")) {
            if (tag_parser.terminating) {
              style &= ~CIG_TEXT_STYLE_ITALIC;
            } else {
              style |= CIG_TEXT_STYLE_ITALIC;
            }
          } else if (!strcasecmp(tag_parser.name, "u")) {
            if (tag_parser.terminating) {
              style &= ~CIG_TEXT_STYLE_UNDERLINE;
            } else {
              style |= CIG_TEXT_STYLE_UNDERLINE;
            }
          } else if (!strcasecmp(tag_parser.name, "s") || !strcasecmp(tag_parser.name, "del")) {
            if (tag_parser.terminating) {
              style &= ~CIG_TEXT_STYLE_STRIKETHROUGH;
            } else {
              style |= CIG_TEXT_STYLE_STRIKETHROUGH;
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
        font = font_stack.count > 0 ? font_stack.fonts[font_stack.count-1] : props.font;
        font_info = font_query(font);
        utf8_string slice = slice_utf8_string(utext, span_start, cur - span_start);
        cig_vec2_t bounds = measure_callback
          ? measure_callback(slice.str, slice.byte_len, font, style)
          : cig_vec2_zero();
        
        label->spans[label->span_count++] = (span_t) { 
          .str = slice.str,
          .font = font_stack.count > 0 ? font_stack.fonts[font_stack.count-1] : NULL,
          .color = color_stack.count > 0 ? color_stack.colors[color_stack.count-1] : NULL,
          .bounds = { bounds.x, bounds.y },
          .byte_len = (unsigned char)slice.byte_len,
          .spacing_after = cp == 0x20 ? font_info.word_spacing : 0,
          .style_flags = style
        };
        
        // printf("appended span %d = %.*s\n", label->span_count-1, label->spans[label->span_count-1].slice.byte_len, label->spans[label->span_count-1].slice.str);
        
        span_start = cur + ch.byte_len;
      }
      
      cur += ch.byte_len;
    }
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
  register span_t *span, *line_start, *line_end, *last = first + count, *prev = NULL;
  register const cig_font_info_t font_info = font_query(props->font);
  
  static double alignment_constant[3] = { 0, 0.5, 1 };
  
  /* Figure out number of lines needed */
  for (span = first, x = 0, y = 0; span < last; span++) {
    if (x + span->bounds.w > absolute_rect.w && x > 0) {
      x = 0;
      y ++;
    }
    
    x += span->bounds.w + span->spacing_after;
  }
  
  lines = 1 + y;
  dy = absolute_rect.y + (int)((absolute_rect.h - (lines * font_info.height)) * alignment_constant[props->alignment.vertical-1]);
  
  for (line_start = span = first, x = 0; span < last;) {
    if (x + span->bounds.w > absolute_rect.w) {
      if (!x) {
        w = span->bounds.w;
        line_end = span+1;
      } else {
        line_end = span;
        w = x - prev->spacing_after;
        x = 0;
      }
    } else if (span == last-1) {
      line_end = last;
      w = x + span->bounds.w;
    } else {
      line_end = NULL;
      x += span->bounds.w + span->spacing_after;
    }
    
    if (line_end) {
      dx = absolute_rect.x + (int)((absolute_rect.w - w) * alignment_constant[props->alignment.horizontal-1]);
      
      for (span = line_start; span < line_end; span++) {
        cig_font_info_t span_font_info = span->font ? font_query(span->font) : font_info;
        
        render_callback(
          span->str,
          span->byte_len,
          cig_rect_make(
            dx,
            dy + (span->font ? ((font_info.height+font_info.baseline_offset)-(span_font_info.height+span_font_info.baseline_offset)) : 0),
            span->bounds.w,
            span->bounds.h
          ),
          span->font ? span->font : props->font,
          span->color ? span->color : props->color,
          span->style_flags
        );
        
        dx += span->bounds.w + span->spacing_after;
      }

      prev = span;
      span = line_start = line_end;
      dy += font_info.height;
      
      continue;
    }
    
    prev = span;
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
      return true;
    }
  } else {
    if (cp == 0x2F) { /* / */
      this->terminating = true;
    } else if (cp == 0x3E) { /* > */
      this->open = false;
      return true;
    } else if (cp == 0x3D && this->_naming) { /* = */
      this->_naming = false; /* Switch to collecting tag value */
    } else if (cp == 0x20) {
      /* Ignore spaces */
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
