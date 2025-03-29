#ifndef CIG_TEXT_INCLUDED
#define CIG_TEXT_INCLUDED

#include "cigcore.h"

typedef void* cig_font_ref;
typedef unsigned int cig_text_color_ref;

typedef struct {
  unsigned int height,
               line_spacing;
} cig_font_info_t;

typedef enum {
  CIG_TEXT_ALIGN_LEFT = 0,
  CIG_TEXT_ALIGN_CENTER,
  CIG_TEXT_ALIGN_RIGHT
} cig_text_horizontal_alignment_t;

typedef enum {
  CIG_TEXT_ALIGN_TOP = 0,
  CIG_TEXT_ALIGN_MIDDLE,
  CIG_TEXT_ALIGN_BOTTOM
} cig_text_vertical_alignment_t;

typedef struct {
  cig_font_ref font;
  cig_text_color_ref color;
  struct {
    cig_text_horizontal_alignment_t horizontal;
    cig_text_vertical_alignment_t vertical;
  } alignment;
} cig_text_properties_t;

/* ┌─────────────────────┐
───┤  BACKEND CALLBACKS  │
   └─────────────────────┘ */

typedef void (*cig_text_render_callback_t)(cig_rect_t, const char*, size_t, cig_font_ref, cig_text_color_ref);
void cig_set_text_render_callback(cig_text_render_callback_t);

typedef cig_vec2_t (*cig_text_measure_callback_t)(const char*, size_t, cig_font_ref);
void cig_set_text_measure_callback(cig_text_measure_callback_t);

typedef cig_font_info_t (*cig_font_query_callback_t)(cig_font_ref);
void cig_set_font_query_callback(cig_font_query_callback_t);

/* ┌────────────────┐
───┤  TEXT DISPLAY  │
   └────────────────┘ */
   
void cig_set_default_font(cig_font_ref);

/* */
void cig_label(const char*, cig_text_properties_t);

#endif
