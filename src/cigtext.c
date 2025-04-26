#include "cigtext.h"
#include "cigcorem.h"
#include "utf8.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define MAX_TAG_NAME_LEN 16
#define MAX_TAG_VALUE_LEN 32

typedef struct {
  bool open,
       terminating,
       _naming;
  char name[MAX_TAG_NAME_LEN],
       value[MAX_TAG_VALUE_LEN];
} tag_parser_t;

typedef struct { unsigned short w, h; } bounds_t;

static cig_text_render_callback_t render_callback = NULL;
static cig_text_measure_callback_t measure_callback = NULL;
static cig_font_query_callback_t font_query = NULL;
static cig_font_ref default_font = 0;
static char printf_buf[CIG_LABEL_PRINTF_BUF_LENGTH];

static void prepare_label(label_t *, cig_text_properties_t *, unsigned int, const char *);
static void render_spans(span_t *, size_t, cig_font_ref, cig_text_color_ref, cig_text_horizontal_alignment_t, cig_text_vertical_alignment_t, bounds_t);
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

/* ┌────────────────┐
───┤  TEXT DISPLAY  │
   └────────────────┘ */

void cig_set_default_font(cig_font_ref font_ref) {
  default_font = font_ref;
}

void cig_label(cig_text_properties_t props, const char *text, ...) {
  register const cig_rect_t absolute_rect = cig_rect_inset(cig_absolute_rect(), cig_frame()->insets);

  label_t *label = CIG_ALLOCATE(label_t);

  if (props.flags & CIG_TEXT_FORMATTED) {
    va_list args;
    va_start(args, text);
    vsprintf(printf_buf, text, args);
    va_end(args);
    prepare_label(label, &props, absolute_rect.w, printf_buf);
  } else {
    prepare_label(label, &props, absolute_rect.w, text);
  }

  if (render_callback) {
    render_spans(
      &label->spans[0],
      label->span_count,
      label->font,
      label->color,
      label->alignment.horizontal,
      label->alignment.vertical,
      (bounds_t) { label->bounds.w, label->bounds.h }
    );
  }
}

label_t* cig_prepare_label(
  label_t *label,
  cig_text_properties_t props,
  unsigned int max_width,
  const char *text,
  ...
) {
  if (props.flags & CIG_TEXT_FORMATTED) {
    va_list args;
    va_start(args, text);
    vsprintf(printf_buf, text, args);
    va_end(args);
    prepare_label(label, &props, max_width, printf_buf);
  } else {
    prepare_label(label, &props, max_width, text);
  }

  return label;
}

void cig_prepared_label(label_t *label) {
  if (render_callback) {
    render_spans(
      &label->spans[0],
      label->span_count,
      label->font,
      label->color,
      label->alignment.horizontal,
      label->alignment.vertical,
      (bounds_t) { label->bounds.w, label->bounds.h }
    );
  }
}

/* ╔════════════════════════════════════════════╗
   ║            INTERNAL FUNCTIONS              ║
   ╚════════════════════════════════════════════╝ */

static void prepare_label(
  label_t *label,
  cig_text_properties_t *props,
  unsigned int max_width,
  const char *str
) {
  cig_id_t hash = cig_hash(str);

  if (label->hash != hash) {
    if (!props->font) {
      props->font = default_font;
    }

    if (props->alignment.horizontal == CIG_TEXT_ALIGN_DEFAULT) {
      props->alignment.horizontal = CIG_TEXT_ALIGN_CENTER;
    }

    if (props->alignment.vertical == CIG_TEXT_ALIGN_DEFAULT) {
      props->alignment.vertical = CIG_TEXT_ALIGN_MIDDLE;
    }

    label->hash = hash;
    label->span_count = 0;
    label->bounds.w = 0;
    label->bounds.h = 0;
    label->font = props->font;
    label->color = props->color;
    label->alignment.horizontal = props->alignment.horizontal;
    label->alignment.vertical = props->alignment.vertical;

    tag_parser_t tag_parser = { 0 };
    utf8_string utext = make_utf8_string(str);
    utf8_char_iter iter = make_utf8_char_iter(utext);
    
    bool tag_stage_changed;
    utf8_char ch;
    uint32_t cp;
    size_t span_start = 0, cur = 0, line_w = 0, line_count = 1;
    cig_font_ref font;
    cig_font_info_t base_font_info = font_query(props->font);
    cig_font_info_t font_info;
    cig_text_style_t style = 0;
    span_t *new_span = 0, *prev_span = 0;
    
    struct { size_t count; cig_font_ref fonts[4]; } font_stack = { 0 };
    struct { size_t count; cig_text_color_ref colors[4]; } color_stack = { 0 };
    
    while ((ch = next_utf8_char(&iter)).byte_len > 0 && (cp = unicode_code_point(ch))) {
      /*printf("character: %.*s\t", (int)ch.byte_len, ch.str);
      printf("unicode code point: U+%04X\t", cp);
      printf("open = %d\t", tag_parser.open);
      printf("span_start = %d\n", (int)span_start);*/
      
      /* Tag was either opened or closed */
      if (props->flags & CIG_TEXT_FORMATTED && (tag_stage_changed = parse_tag(&tag_parser, ch, cp))) {
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
              style &= ~CIG_TEXT_BOLD;
            } else {
              style |= CIG_TEXT_BOLD;
            }
          } else if (!strcasecmp(tag_parser.name, "i")) {
            if (tag_parser.terminating) {
              style &= ~CIG_TEXT_ITALIC;
            } else {
              style |= CIG_TEXT_ITALIC;
            }
          } else if (!strcasecmp(tag_parser.name, "u")) {
            if (tag_parser.terminating) {
              style &= ~CIG_TEXT_UNDERLINE;
            } else {
              style |= CIG_TEXT_UNDERLINE;
            }
          } else if (!strcasecmp(tag_parser.name, "s") || !strcasecmp(tag_parser.name, "del")) {
            if (tag_parser.terminating) {
              style &= ~CIG_TEXT_STRIKETHROUGH;
            } else {
              style |= CIG_TEXT_STRIKETHROUGH;
            }
          }
        }
      } else if (tag_parser.open) {
        cur += ch.byte_len;
        continue;
      }
      
      /* Span terminated:
         a) basic space character
         b) tag encountered
         c) end of string */
      if ((cur > span_start && (cp == 0x20 || tag_stage_changed)) || (cur + ch.byte_len == utext.byte_len)) {
        if (cur + ch.byte_len == utext.byte_len) {
          cur += ch.byte_len;
        }
        font = font_stack.count > 0 ? font_stack.fonts[font_stack.count-1] : props->font;
        font_info = font_query(font);
        utf8_string slice = slice_utf8_string(utext, span_start, cur - span_start);
        cig_vec2_t bounds = measure_callback
          ? measure_callback(slice.str, slice.byte_len, font, style)
          : cig_vec2_zero();

        prev_span = new_span;
        new_span = &label->spans[label->span_count++];
        *new_span = (span_t) { 
          .str = slice.str,
          .font = font_stack.count > 0 ? font_stack.fonts[font_stack.count-1] : 0,
          .color = color_stack.count > 0 ? color_stack.colors[color_stack.count-1] : 0,
          .bounds = { bounds.x, bounds.y },
          .byte_len = (unsigned char)slice.byte_len,
          .spacing_after = cp == 0x20 ? font_info.word_spacing : 0,
          .style_flags = props->style | style,
          .newlines = 0
        };

        if (max_width) {
          if (line_w + bounds.x + (prev_span ? prev_span->spacing_after : 0) > max_width) { /* Push span to new line */
            if (prev_span) {
              prev_span->newlines = 1;
              prev_span->spacing_after = 0;
              line_count ++;
            }
            if (!line_w) { /* This is the first span on this line, no line change needed */
              label->bounds.w = CIG_MAX(label->bounds.w, bounds.x);
              new_span->newlines = 0;
              new_span->spacing_after = 0;
            } else {
              line_w = bounds.x; /* New line started */
            }
          } else {
            if (prev_span) {
              line_w += prev_span->spacing_after;
            }
            line_w += bounds.x;
            label->bounds.w = CIG_MAX(label->bounds.w, line_w);
          }
        } else {
          if (prev_span) {
            line_w += prev_span->spacing_after;
          }
          line_w += bounds.x;
          label->bounds.w = CIG_MAX(label->bounds.w, line_w);
        }

        // printf("appended span %d = %.*s\n", label->span_count-1, label->spans[label->span_count-1].slice.byte_len, label->spans[label->span_count-1].slice.str);
        
        span_start = cur + ch.byte_len;
      }
      
      cur += ch.byte_len;
    }

    line_count = CIG_MAX(1, line_count);
    label->bounds.h = (line_count * base_font_info.height) + (line_count - 1) * base_font_info.line_spacing;
  }
}

static void render_spans(
  span_t *first,
  size_t count,
  cig_font_ref base_font,
  cig_text_color_ref base_color,
  cig_text_horizontal_alignment_t horizontal_alignment,
  cig_text_vertical_alignment_t vertical_alignment,
  bounds_t bounds
) {
  if (!count) { return; }

  register const cig_rect_t absolute_rect = cig_rect_inset(cig_absolute_rect(), cig_frame()->insets);
  register int lines = 1, w, dx, dy;
  register span_t *span, *line_start, *line_end, *last = first + (count-1);
  register const cig_font_info_t font_info = font_query(base_font);
  
  static double alignment_constant[3] = { 0, 0.5, 1 };
  
  /* Add up number of lines needed */
  for (span = first; span <= last; span++) {
    lines += span->newlines;
  }
  
  dy = absolute_rect.y + (int)((absolute_rect.h - bounds.h) * alignment_constant[vertical_alignment-1]);
  line_start = span = first;
  w = 0;

  while (span <= last) {
    w += span->bounds.w + span->spacing_after;

    if (span->newlines || span == last) {
      line_end = span;
      dx = absolute_rect.x + (int)((absolute_rect.w - w) * alignment_constant[horizontal_alignment-1]);

      for (span = line_start; span <= line_end; span++) {
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
          span->font ? span->font : base_font,
          span->color ? span->color : base_color,
          span->style_flags
        );
        
        dx += span->bounds.w + span->spacing_after;
      }

      dy += line_end->newlines * font_info.height;
      w = 0;
      span = line_start = line_end+1;
      
      continue;
    }

    span++;
  }



  /*for (line_start = span = first, x = 0; span < last;) {
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
  }*/
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
