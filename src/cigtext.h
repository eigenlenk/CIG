#ifndef CIG_TEXT_INCLUDED
#define CIG_TEXT_INCLUDED

#include "cigcore.h"

/* ┌─────────────────────┐
───┤  BACKEND CALLBACKS  │
   └─────────────────────┘ */

typedef void (*cig_text_render_callback_t)(cig_rect_t, const char*, size_t);
void cig_set_text_render_callback(cig_text_render_callback_t);

typedef cig_vec2_t (*cig_text_measure_callback_t)(const char*, size_t);
void cig_set_text_measure_callback(cig_text_measure_callback_t);

/* ┌────────────────┐
───┤  TEXT DISPLAY  │
   └────────────────┘ */

/* Basic text element that's vertically centered in its frame */
void cig_label(const char*);

#endif
