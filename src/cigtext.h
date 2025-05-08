#ifndef CIG_TEXT_INCLUDED
#define CIG_TEXT_INCLUDED

#include "cigcore.h"

#define CIG_LABEL_SPANS_MAX 48
#define CIG_LABEL_PRINTF_BUF_LENGTH 512

typedef void* cig_font_ref;
typedef void* cig_text_color_ref;

typedef struct {
  int height,
      baseline_offset;
} cig_font_info_t;

#define CIG_TEXT_ALIGN_DEFAULT 0

typedef enum {
  CIG_TEXT_ALIGN_LEFT = 1,
  CIG_TEXT_ALIGN_CENTER,
  CIG_TEXT_ALIGN_RIGHT
} cig_text_horizontal_alignment_t;

typedef enum {
  CIG_TEXT_ALIGN_TOP = 1,
  CIG_TEXT_ALIGN_MIDDLE,
  CIG_TEXT_ALIGN_BOTTOM
} cig_text_vertical_alignment_t;

typedef enum {
  CIG_TEXT_BOLD = CIG_BIT(0),
  CIG_TEXT_ITALIC = CIG_BIT(1),
  CIG_TEXT_UNDERLINE = CIG_BIT(2),
  CIG_TEXT_STRIKETHROUGH = CIG_BIT(3)
} cig_text_style_t;

typedef enum {
  CIG_TEXT_OVERFLOW = 0,  /* Text is allowed to flow out of bounds */
  CIG_TEXT_TRUNCATE,      /* The last non-fitting word is truncated */
  CIG_TEXT_SHOW_ELLIPSIS
} cig_text_overflow_t;

typedef struct {
  cig_font_ref font;
  cig_text_color_ref color;
  struct {
    cig_text_horizontal_alignment_t horizontal;
    cig_text_vertical_alignment_t vertical;
  } alignment;
  int max_lines,
      line_spacing;
  cig_text_overflow_t overflow;
  enum {
    CIG_TEXT_FORMATTED = CIG_BIT(0)
  } flags;
  cig_text_style_t style;
} cig_text_properties_t;

/*  Span is an atomic text component, a piece of text that runs until
    the horizontal bounds of the label, or until some property of the
    text changes (font, color, link etc.) */
typedef struct {
  const char *str;
  CIG_OPTIONAL(cig_font_ref) font_override;
  CIG_OPTIONAL(cig_text_color_ref) color_override;
  struct { unsigned short w, h; } bounds;
  unsigned char byte_len;
  unsigned char style_flags;
  unsigned char newlines;
} cig_span_t;

typedef struct {
  cig_span_t spans[CIG_LABEL_SPANS_MAX];
  struct {
    cig_text_horizontal_alignment_t horizontal;
    cig_text_vertical_alignment_t vertical;
  } alignment;
  cig_id_t hash;
  cig_font_ref font;
  cig_text_color_ref color;
  struct { unsigned short w, h; } bounds;
  unsigned char span_count;
  unsigned char line_count;
  char line_spacing;
} cig_label_t;

typedef void (*cig_text_render_callback_t)(const char*, size_t, cig_r, cig_font_ref, cig_text_color_ref, cig_text_style_t);
typedef cig_v (*cig_text_measure_callback_t)(const char*, size_t, cig_font_ref, cig_text_style_t);
typedef cig_font_info_t (*cig_font_query_callback_t)(cig_font_ref);

/*  ┌───────────────────┐
    │ BACKEND CALLBACKS │
    └───────────────────┘ */

void cig_set_text_render_callback(cig_text_render_callback_t);

void cig_set_text_measure_callback(cig_text_measure_callback_t);

void cig_set_font_query_callback(cig_font_query_callback_t);

/*  ┌──────────────┐
    │ TEXT DISPLAY │
    └──────────────┘ */

void cig_set_default_font(cig_font_ref);

void cig_set_default_text_color(cig_text_color_ref);

/* */
cig_label_t* cig_label(cig_text_properties_t, const char*, ...);

/*  For more advanced text display you can prepare a piece of text.
    This enables accessing the text bounds before rendering it to
    pass as a size for the next layout frame for example. It also exposes
    the underlying spans (smallest text components) */
cig_label_t* cig_prepare_label(cig_label_t *, unsigned int, cig_text_properties_t, const char *, ...);

/*  Renders a prepared label */
void cig_draw_label(cig_label_t *);

#endif
