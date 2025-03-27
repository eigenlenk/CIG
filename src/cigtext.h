#ifndef CIG_TEXT_INCLUDED
#define CIG_TEXT_INCLUDED

#include "cigcore.h"

typedef void* cig_font_ref;

typedef struct {
  unsigned int height,
               line_spacing;
} cig_font_info_t;

/* ┌─────────────────────┐
───┤  BACKEND CALLBACKS  │
   └─────────────────────┘ */

typedef void (*cig_text_render_callback_t)(cig_rect_t, const char*, size_t);
void cig_set_text_render_callback(cig_text_render_callback_t);

typedef cig_vec2_t (*cig_text_measure_callback_t)(const char*, size_t);
void cig_set_text_measure_callback(cig_text_measure_callback_t);

typedef cig_font_info_t (*cig_font_query_callback_t)(cig_font_ref);
void cig_set_font_query_callback(cig_font_query_callback_t);

/* ┌────────────────┐
───┤  TEXT DISPLAY  │
   └────────────────┘ */
   
void cig_set_default_font(cig_font_ref);

/* Basic text element that's vertically centered in its frame */
void cig_label(const char*);

#endif
