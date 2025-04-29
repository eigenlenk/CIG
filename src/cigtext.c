#include "cigtext.h"
#include "cigcorem.h"
#include "ciggfx.h"
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
static struct { size_t count; cig_font_ref fonts[4]; } font_stack;
static struct { size_t count; cig_text_color_ref colors[4]; } color_stack;
static cig_text_style_t style;

static void prepare_label(label_t *, cig_text_properties_t *, unsigned int, const char *);
static void render_spans(span_t *, size_t, cig_font_ref, cig_text_color_ref, cig_text_horizontal_alignment_t, cig_text_vertical_alignment_t, bounds_t);
static int wrap_text_if_needed(label_t *, utf8_string, cig_text_properties_t *, span_t *, span_t *, size_t, size_t, cig_vec2_t, cig_font_ref, size_t *, unsigned int);
static bool parse_tag(tag_parser_t*, utf8_char, uint32_t);
static void handle_tag(tag_parser_t*);

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
    size_t span_start = 0, cur = 0, next, line_w = 0, line_count = 1, word_len;
    cig_font_ref font;
    cig_font_info_t base_font_info = font_query(props->font);
    cig_font_info_t font_info;
    span_t *new_span = 0, *prev_span = 0;

    style = 0;
    color_stack.count = 0;
    font_stack.count = 0;
    
    while ((ch = next_utf8_char(&iter)).byte_len > 0 && (cp = unicode_code_point(ch))) {
      // printf("character: %.*s\t", (int)ch.byte_len, ch.str);
      // printf("unicode code point: U+%04X\t\n", cp);

      /* Tag was either opened or closed */
      if (props->flags & CIG_TEXT_FORMATTED && (tag_stage_changed = parse_tag(&tag_parser, ch, cp))) {
        /* Tag closed */
        if (!tag_parser.open) {
          span_start = cur + ch.byte_len;
          handle_tag(&tag_parser);
        }
      } else if (tag_parser.open) {
        cur += ch.byte_len;
        continue;
      }
      
      bool is_newline = cp == 0x0A;

      /* Span terminated:
         a) basic space character (0x20)
         b) newline (0x0A)
         c) tag encountered
         d) end of string */
      if ((cur > span_start && (cp == 0x20 || is_newline || tag_stage_changed)) || (cur + ch.byte_len == utext.byte_len)) {
        if (cur + ch.byte_len == utext.byte_len) {
          cur = utext.byte_len;
        } else {
          next = cur + ch.byte_len;
        }

        font = font_stack.count > 0 ? font_stack.fonts[font_stack.count-1] : props->font;
        font_info = font_query(font);
        word_len = cur - span_start;
        utf8_string slice = slice_utf8_string(utext, span_start, word_len);
        cig_vec2_t bounds = measure_callback(slice.str, slice.byte_len, font, style);

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
          .newlines = is_newline ? 1 : 0
        };

        if (is_newline) { /* Look ahead and add any additional newlines */
          line_count ++;
          char *last_valid_pos = (char *)iter.str;
          while ((ch = next_utf8_char(&iter)).byte_len > 0) {
            if (unicode_code_point(ch) == 0x0A) {
              new_span->newlines ++;
              next += ch.byte_len;
              last_valid_pos = (char *)iter.str;
              line_count ++;
            } else {
              iter.str = last_valid_pos;
              break;
            }
          }
        }

        /* Perform text wrapping if needed */
        if (max_width) {
          /* Returns >= 0 number of lines wrapped, or -1 if text was terminated due to wrapping mode */
          const int change = wrap_text_if_needed(label, utext, props, prev_span, new_span, span_start, word_len, bounds, font, &line_w, max_width);
          if (change < 0) {
            goto end_of_loop;
          } else {
            line_count += change;
          }
        } else {
          if (prev_span) {
            line_w += prev_span->spacing_after;
          }
          line_w += bounds.x;
          label->bounds.w = CIG_MAX(label->bounds.w, line_w);
        }

        if (is_newline) { /* Forced newline resets next line width */
          line_w = 0;
        }

        cur = next;
        span_start = next;

        continue;
      }
      
      cur += ch.byte_len;
    }

    end_of_loop:
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
}

static int wrap_text_if_needed(
  label_t *label,
  utf8_string original_string,
  cig_text_properties_t *props,
  span_t *prev_span,
  span_t *new_span,
  size_t span_start,
  size_t word_len,
  cig_vec2_t bounds,
  cig_font_ref current_font,
  size_t *line_w,
  const unsigned int max_width
) {
  const int added_width = bounds.x + (prev_span ? prev_span->spacing_after : 0);

  if (*line_w + added_width > max_width) {
    switch (props->overflow) {
      case CIG_TEXT_WORD_WRAP: {
        /* Push span to new line */
        if (prev_span) {
          prev_span->newlines = 1;
          prev_span->spacing_after = 0;
        }
        if (*line_w == 0) { /* This is the first span on this line, no line change needed */
          label->bounds.w = CIG_MAX(label->bounds.w, bounds.x);
          new_span->newlines = 0;
          new_span->spacing_after = 0;
        } else {
          *line_w = bounds.x; /* New line started */
        }
      } break;

      case CIG_TEXT_SHOW_ELLIPSIS: {
        /* How much of the text overflows? */
        const cig_vec2_t ellipsis_size = measure_callback("...", 3, current_font, style);
        const float overflow = (*line_w+added_width+ellipsis_size.x)-max_width;
        const float overflow_amount = 1.0f-(overflow/added_width);
        utf8_string slice = slice_utf8_string(original_string, span_start, word_len*overflow_amount);
        cig_vec2_t bounds = measure_callback(slice.str, slice.byte_len, current_font, style);
        new_span->byte_len = slice.byte_len;
        new_span->bounds.w = bounds.x;
        new_span->bounds.h = bounds.y;
        new_span->spacing_after = 0;
        label->spans[label->span_count++] = (span_t) { 
          .str = "...",
          .font = new_span->font,
          .color = new_span->color,
          .bounds = { ellipsis_size.x, ellipsis_size.y },
          .byte_len = 3,
          .spacing_after = 0,
          .style_flags = props->style | style,
          .newlines = 0
        };
        *line_w += bounds.x+ellipsis_size.x;
        label->bounds.w = CIG_MAX(label->bounds.w, *line_w);
        return -1;
      };

      case CIG_TEXT_TRUNCATE: {
        /* How much of the text overflows? */
        const float overflow = (*line_w+added_width)-max_width;
        const float overflow_amount = 1.0f-(overflow/added_width);
        utf8_string slice = slice_utf8_string(original_string, span_start, word_len*overflow_amount);
        cig_vec2_t bounds = measure_callback(slice.str, slice.byte_len, current_font, style);
        new_span->byte_len = slice.byte_len;
        new_span->bounds.w = bounds.x;
        new_span->bounds.h = bounds.y;
        *line_w += bounds.x;
        label->bounds.w = CIG_MAX(label->bounds.w, *line_w);
        return -1;
      };
    }
  } else {
    if (prev_span) {
      *line_w += prev_span->spacing_after;
    }
    *line_w += bounds.x;
    label->bounds.w = CIG_MAX(label->bounds.w, *line_w);
  }

  return prev_span ? (int)prev_span->newlines : 0;
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

static void handle_tag(tag_parser_t *tag) {
  if (!strcasecmp(tag->name, "font")) {
    if (tag->terminating) {
      font_stack.count--;
    } else {
      font_stack.fonts[font_stack.count++] = (cig_font_ref)strtol(tag->value, NULL, 16);
    }
  } else if (!strcasecmp(tag->name, "color")) {
    if (tag->terminating) {
      color_stack.count--;
    } else {
      color_stack.colors[color_stack.count++] = (cig_text_color_ref)strtol(tag->value, NULL, 16);
    }
  } else if (!strcasecmp(tag->name, "b")) {
    if (tag->terminating) {
      style &= ~CIG_TEXT_BOLD;
    } else {
      style |= CIG_TEXT_BOLD;
    }
  } else if (!strcasecmp(tag->name, "i")) {
    if (tag->terminating) {
      style &= ~CIG_TEXT_ITALIC;
    } else {
      style |= CIG_TEXT_ITALIC;
    }
  } else if (!strcasecmp(tag->name, "u")) {
    if (tag->terminating) {
      style &= ~CIG_TEXT_UNDERLINE;
    } else {
      style |= CIG_TEXT_UNDERLINE;
    }
  } else if (!strcasecmp(tag->name, "s") || !strcasecmp(tag->name, "del")) {
    if (tag->terminating) {
      style &= ~CIG_TEXT_STRIKETHROUGH;
    } else {
      style |= CIG_TEXT_STRIKETHROUGH;
    }
  } else {
    // Log warning?
  }
}