#ifndef CIG_TEXT_INCLUDED
#define CIG_TEXT_INCLUDED

#include "cigcore.h"
#include "utf8.h"
#include <stdint.h>

#define CIG_LABEL_SPANS_MAX 48
#define CIG_LABEL_PRINTF_BUF_LENGTH 4096
#define CIG_LABEL_SIZEOF(NUM_SPANS) (sizeof(cig_label) + sizeof(cig_span[NUM_SPANS]))
#define CIG_RAW_TEXT_AUTOMATIC_SIZE cig_v_zero()

typedef void* cig_font_ref;
typedef void* cig_text_color_ref;

typedef struct {
  int height,
      baseline_offset;
} cig_font_info_st;

#define CIG_TEXT_ALIGN_DEFAULT 0

typedef enum CIG_PACKED {
  CIG_TEXT_ALIGN_LEFT = 1,
  CIG_TEXT_ALIGN_CENTER,
  CIG_TEXT_ALIGN_RIGHT
} cig_text_horizontal_alignment;

typedef enum CIG_PACKED {
  CIG_TEXT_ALIGN_TOP = 1,
  CIG_TEXT_ALIGN_MIDDLE,
  CIG_TEXT_ALIGN_BOTTOM
} cig_text_vertical_alignment;

typedef enum CIG_PACKED {
  CIG_TEXT_BOLD = CIG_BIT(0),
  CIG_TEXT_ITALIC = CIG_BIT(1),
  CIG_TEXT_UNDERLINE = CIG_BIT(2),
  CIG_TEXT_STRIKETHROUGH = CIG_BIT(3)
} cig_text_style;

typedef enum CIG_PACKED {
  CIG_TEXT_OVERFLOW = 0,  /* Text is allowed to flow out of bounds */
  CIG_TEXT_TRUNCATE,      /* The last non-fitting word is truncated */
  CIG_TEXT_SHOW_ELLIPSIS
} cig_text_overflow;

typedef struct {
  cig_font_ref font;
  cig_text_color_ref color;
  struct {
    cig_text_horizontal_alignment horizontal;
    cig_text_vertical_alignment vertical;
  } alignment;
  uint32_t max_lines;
  int32_t line_spacing;
  cig_text_overflow overflow;
  enum CIG_PACKED {
    CIG_TEXT_FORMATTED = CIG_BIT(0),
    CIG_TEXT_HORIZONTAL_WRAP_DISABLED = CIG_BIT(1), /*  */
    CIG_TEXT_VERTICAL_CLIPPING_ENABLED = CIG_BIT(2), /* New lines are added until they fit perfectly vertically */
    CIG_TEXT_UNIQUE_STRING_POINTER = CIG_BIT(3) /* String pointer address is used to identify text and observe changes */
  } flags;
  cig_text_style style;
} cig_text_properties;

/*
 * Span is an atomic text component, a piece of text that runs until
 * the horizontal bounds of the label, or until some property of the
 * text changes (font, color, link etc.)
 */
typedef struct {
  const char *str;
  CIG_OPTIONAL(cig_font_ref) font_override;
  CIG_OPTIONAL(cig_text_color_ref) color_override;
  struct { unsigned short w, h; } bounds;
  unsigned short byte_len;
  unsigned char style_flags;
  unsigned char newlines;
} cig_span;

typedef struct {
  struct {
    cig_text_horizontal_alignment horizontal;
    cig_text_vertical_alignment vertical;
  } alignment;
  cig_id hash;
  cig_font_ref font;
  cig_text_color_ref color;
  struct { unsigned short w, h; } bounds;
  size_t available_spans;
  unsigned short span_count;
  unsigned short line_count;
  char line_spacing;
  cig_span spans[];
} cig_label;

typedef void (*cig_draw_text_callback)(const char *, size_t, cig_r, cig_font_ref, cig_text_color_ref, cig_text_style);
typedef cig_v (*cig_measure_text_callback)(const char *, size_t, cig_font_ref, cig_text_style);
typedef cig_font_info_st (*cig_query_font_callback)(cig_font_ref);

/*
 * ┌───────────────────┐
 * │ BACKEND CALLBACKS │
 * └───────────────────┘
 */

void cig_assign_draw_text(cig_draw_text_callback);

void cig_assign_measure_text(cig_measure_text_callback);

void cig_assign_query_font(cig_query_font_callback);

/*
 * ┌──────────────┐
 * │ TEXT DISPLAY │
 * └──────────────┘
 */

void
cig_set_default_font(cig_font_ref);

void
cig_set_default_text_color(cig_text_color_ref);

cig_font_info_st
cig_font_info(cig_font_ref);

/*
 * Allocates a label type in the current element's state, prepares it and
 * draws it. Label is cached based on the input string hash.
 */
CIG_DISCARDABLE(cig_label*)
cig_draw_label(cig_text_properties, const char*, ...);

/*
 * For more advanced text display you can prepare a piece of text.
 * This enables accessing the text bounds before rendering it to
 * pass as a size for the next layout frame for example. It also exposes
 * the underlying spans (smallest text components)
 */
cig_label*
cig_label_prepare(cig_label*, cig_v, cig_text_properties, const char*, ...);

/* Renders a prepared label */
void
cig_label_draw(cig_label*);

/* */
char*
cig_utf8_string_line_location(utf8_string, const char*, int16_t line_offset);

/*=================================================================

   Raw text API allows drawing simple pieces of text
   without storing anything internally.

  =================================================================*/

cig_v
cig_measure_raw_text(cig_font_ref, cig_text_style, const char*);

cig_v
cig_measure_raw_text_formatted(cig_font_ref, cig_text_style, const char*, ...);

void
cig_draw_raw_text(cig_v, cig_v, cig_font_ref, cig_text_style, cig_text_color_ref, const char*);

void
cig_draw_raw_text_formatted(cig_v, cig_v, cig_font_ref, cig_text_style, cig_text_color_ref, const char*, ...);

#endif
