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
static cig_text_color_ref default_text_color = 0;
static char printf_buf[CIG_LABEL_PRINTF_BUF_LENGTH];
static struct { size_t count; cig_font_ref fonts[4]; } font_stack;
static struct { size_t count; cig_text_color_ref colors[4]; } color_stack;
static cig_text_style_t style;

static void prepare_label(label_t *, const cig_text_properties_t *, const unsigned int, const char *);
static span_t* create_span(label_t *, utf8_string, cig_font_ref, cig_text_color_ref, cig_text_style_t, cig_vec2_t);
static void render_spans(span_t *, size_t, cig_font_ref, cig_text_color_ref, cig_text_horizontal_alignment_t, cig_text_vertical_alignment_t, bounds_t);
static void wrap_text(utf8_string *, size_t, cig_vec2_t *, cig_text_overflow_t, cig_font_ref, cig_font_ref, cig_text_color_ref, cig_text_style_t, size_t, span_t *);
static bool parse_tag(tag_parser_t*, utf8_char, uint32_t);
static void apply_tag(tag_parser_t*);

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

void cig_set_default_font(cig_font_ref font) {
  default_font = font;
}

void cig_set_default_text_color(cig_text_color_ref color) {
  default_text_color = color;
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
  const cig_text_properties_t *props,
  const unsigned int max_width,
  const char *str
) {
  cig_id_t hash = cig_hash(str);

  if (label->hash != hash) {
    label->hash = hash;
    label->span_count = 0;
    label->bounds.w = 0;
    label->bounds.h = 0;
    label->font = props->font ? props->font : default_font;
    label->color = props->color ? props->color : default_text_color;
    label->alignment.horizontal = props->alignment.horizontal == CIG_TEXT_ALIGN_DEFAULT
      ? CIG_TEXT_ALIGN_CENTER
      : props->alignment.horizontal;
    label->alignment.vertical = props->alignment.vertical == CIG_TEXT_ALIGN_DEFAULT
      ? CIG_TEXT_ALIGN_MIDDLE
      : props->alignment.vertical;

    utf8_string utext = make_utf8_string(str);
    utf8_char_iter iter = make_utf8_char_iter(utext);
    utf8_char ch;
    uint32_t cp;
    size_t i = 0, line_width = 0, line_count = 1;

    cig_font_info_t base_font_info = font_query(label->font);

    tag_parser_t tag_parser = { 0 };
    bool tag_stage_changed = false;

    struct {
      bool reading;
      size_t start;
      struct {
        char *str;
        size_t index;
        utf8_string slice;
        cig_vec2_t bounds;
      } last_fitting;
    } span = { 0 };

    style = 0;
    color_stack.count = 0;
    font_stack.count = 0;

    // printf("Label: |%s|\n", str);

    while ((ch = next_utf8_char(&iter)).byte_len > 0 && (cp = unicode_code_point(ch))) {
      /* Tag was either opened or closed */
      if (props->flags & CIG_TEXT_FORMATTED && (tag_stage_changed = parse_tag(&tag_parser, ch, cp))) {
        if (!tag_parser.open) { /* Tag closed */
          apply_tag(&tag_parser);
        }
        if (!span.reading) {
          i += ch.byte_len;
          continue;
        }
      } else if (tag_parser.open) {
        i += ch.byte_len;
        continue;
      } else if (!span.reading) {
        // printf("Setting span start to %i\n", i);
        span.reading = true;
        span.start = i;
      }

      const bool is_newline = cp == 0x0A;
      const bool is_space = cp == 0x20;
      const bool end_of_string = (i + ch.byte_len == utext.byte_len);
      const bool terminates_span = (i > span.start && (is_newline || tag_stage_changed)) || (end_of_string && (i = utext.byte_len));

      if ((max_width && is_space) || terminates_span) {
        size_t length = i - span.start;
        utf8_string slice = slice_utf8_string(utext, span.start, length);
        cig_text_color_ref color_override = color_stack.count > 0 ? color_stack.colors[color_stack.count-1] : 0;
        cig_font_ref font_override = font_stack.count > 0 ? font_stack.fonts[font_stack.count-1] : 0;
        cig_font_ref display_font = font_override ? font_override : label->font;
        cig_vec2_t bounds = measure_callback(slice.str, slice.byte_len, display_font, style);
        size_t newlines = is_newline ? 1 : 0;

        if (is_newline) { /* Look ahead and count all additional newlines */
          char *last_valid_pos = (char *)iter.str;
          size_t last_valid_index = i;
          while ((ch = next_utf8_char(&iter)).byte_len > 0) {
            if (unicode_code_point(ch) == 0x0A) {
              newlines ++;
              last_valid_pos = (char *)iter.str;
              last_valid_index = i = i + ch.byte_len;
            } else {
              iter.str = last_valid_pos;
              i = last_valid_index;
              break;
            }
          }

          // printf("\tNewlines: %i\n", newlines);
        }

        // printf("Checking [%i...%i] (w = %i/%i, bx = %i): |%.*s|\n", span.start, i, line_width, max_width, bounds.x, (int)length, &str[span.start]);

        if (max_width) {
          if (line_width + bounds.x <= max_width) {
            span.last_fitting.str = (char*)iter.str;
            span.last_fitting.index = i;
            span.last_fitting.slice = slice;
            span.last_fitting.bounds = bounds;

            if (terminates_span) {
              // printf("\tForced end of span (1)!\n");
              span_t *new_span = create_span(label, slice, font_override, color_override, props->style | style, bounds);
              line_count += new_span->newlines = newlines;
              span.last_fitting.str = NULL;
              span.reading = false;
              line_width += bounds.x;
              label->bounds.w = CIG_MAX(label->bounds.w, line_width);
            }
          } else {
            if (span.last_fitting.str && (!props->max_lines || line_count < props->max_lines)) {
              // printf("\tNo fit. Use last range [%i...%i]: |%.*s|\n", span.start, span.last_fitting.index, span.last_fitting.slice.byte_len, span.last_fitting.slice.str);
              span_t *new_span = create_span(label, span.last_fitting.slice, font_override, color_override, props->style | style, span.last_fitting.bounds);
              line_count += new_span->newlines = 1;
              iter.str = span.last_fitting.str;
              i = span.last_fitting.index;
              label->bounds.w = CIG_MAX(label->bounds.w, span.last_fitting.bounds.x);
              line_width = 0;
              span.last_fitting.str = NULL;
              span.reading = false;
            } else {
              if (!end_of_string && props->max_lines == line_count && props->overflow == CIG_TEXT_OVERFLOW) {
                /* All text will be place on one line, no matter it going out of bounds */
                i += ch.byte_len;
                continue;
              }
              // printf("\tForcing a span!\n");
              span_t additional_span = { 0 };
              if (bounds.x > max_width && (!props->max_lines || props->max_lines == line_count)) {
                wrap_text(&slice, length, &bounds, props->overflow, display_font, font_override, color_override, props->style | style, max_width, &additional_span);
              }
              span_t *new_span = create_span(label, slice, font_override, color_override, props->style | style, bounds);
              if (additional_span.str) {
                label->spans[label->span_count++] = additional_span;
              } else if (!end_of_string) {
                line_count += new_span->newlines = 1;
              }
              span.last_fitting.str = NULL;
              span.reading = false;
              line_width = 0;
              label->bounds.w = CIG_MAX(label->bounds.w, bounds.x);
            }
          }
        } else {
          // printf("\tForced end of span (2)!\n");
          create_span(label, slice, font_override, color_override, props->style | style, bounds);
          label->bounds.w = CIG_MAX(label->bounds.w, bounds.x);
          span.reading = false;
        }

        if (newlines) {
          line_width = 0;
        }
      }

      i += ch.byte_len;
    }

    line_count = CIG_MAX(1, line_count);
    label->line_count = line_count;
    label->bounds.h = (line_count * base_font_info.height) + (line_count - 1) * base_font_info.line_spacing;
  }
}

static void wrap_text(
  utf8_string *slice,
  size_t text_length,
  cig_vec2_t *bounds,
  cig_text_overflow_t overflow_mode,
  cig_font_ref display_font,
  cig_font_ref font_override,
  cig_text_color_ref color_override,
  cig_text_style_t style,
  size_t max_width,
  span_t *additional_span
) {
  switch (overflow_mode) {
    case CIG_TEXT_SHOW_ELLIPSIS: {
      /* How much of the text overflows? */
      const cig_vec2_t ellipsis_size = measure_callback("...", 3, display_font, style);
      const double overflow = bounds->x - (max_width - ellipsis_size.x);
      const double truncation_amount = 1.0 - (overflow / bounds->x);
      /* This is a guesstimate and may not still fit properly */
      int new_length = round((double)text_length * truncation_amount);
      *slice = slice_utf8_string(*slice, 0, new_length);
      *bounds = measure_callback(slice->str, slice->byte_len, display_font, style);
      while (new_length > 0 && bounds->x + ellipsis_size.x > max_width) { /* Shorten even more */
        new_length -= 1;
        *slice = slice_utf8_string(*slice, 0, new_length);
        *bounds = measure_callback(slice->str, slice->byte_len, display_font, style);
      }
      *additional_span = (span_t) { 
        .str = "...",
        .font = font_override,
        .color = color_override,
        .bounds = { ellipsis_size.x, ellipsis_size.y },
        .byte_len = 3,
        .style_flags = style,
        .newlines = 0
      };
    } break;

    case CIG_TEXT_TRUNCATE: {
      /* How much of the text overflows? */
      const double overflow = bounds->x - max_width;
      const double truncation_amount = 1.0 - (overflow / bounds->x);
      int new_length = round((double)text_length * truncation_amount);
      *slice = slice_utf8_string(*slice, 0, new_length);
      *bounds = measure_callback(slice->str, slice->byte_len, display_font, style);
      while (new_length > 0 && bounds->x > max_width) { /* Shorten even more */
        new_length -= 1;
        *slice = slice_utf8_string(*slice, 0, new_length);
        *bounds = measure_callback(slice->str, slice->byte_len, display_font, style);
      }
    } break;

    default: break;
  }
}

static span_t* create_span(
  label_t *label,
  utf8_string slice,
  cig_font_ref font_override,
  cig_text_color_ref color_override,
  cig_text_style_t style,
  cig_vec2_t bounds
) {
  label->spans[label->span_count++] = (span_t) { 
    .str = slice.str,
    .font = font_override,
    .color = color_override,
    .bounds = { bounds.x, bounds.y },
    .byte_len = (unsigned char)slice.byte_len,
    .style_flags = style,
    .newlines = 0
  };

  return &label->spans[label->span_count-1];
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
  register int w, dx, dy;
  register span_t *span, *line_start, *line_end, *last = first + (count-1);
  register const cig_font_info_t font_info = font_query(base_font);
  
  static double alignment_constant[3] = { 0, 0.5, 1 };
  
  dy = absolute_rect.y + (int)((absolute_rect.h - bounds.h) * alignment_constant[vertical_alignment-1]);
  line_start = span = first;
  w = 0;

  while (span <= last) {
    w += span->bounds.w;

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
        
        dx += span->bounds.w;
      }

      dy += line_end->newlines * font_info.height;
      w = 0;
      span = line_start = line_end+1;
      
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

static void apply_tag(tag_parser_t *tag) {
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