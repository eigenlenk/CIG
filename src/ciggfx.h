#ifndef CIG_GFX_INCLUDED
#define CIG_GFX_INCLUDED

#include "cigcore.h"

typedef void* cig_color_ref;
typedef void* cig_panel_ref;
typedef void* cig_image_ref;

typedef enum {
  CIG_PANEL_HOVERED = CIG_BIT(0),
  CIG_PANEL_PRESSED = CIG_BIT(1),
  CIG_PANEL_FOCUSED = CIG_BIT(2)
} cig_panel_modifiers_t;

/* ┌─────────────────────┐
───┤  BACKEND CALLBACKS  │
   └─────────────────────┘ */

typedef void (*cig_panel_render_callback_t)(cig_panel_ref, cig_rect_t, cig_panel_modifiers_t);
void cig_set_panel_render_callback(cig_panel_render_callback_t);

typedef void (*cig_draw_rectangle_callback_t)(cig_color_ref, cig_color_ref, cig_rect_t, unsigned int);
void cig_set_draw_rectangle_callback(cig_draw_rectangle_callback_t);

typedef void (*cig_draw_line_callback_t)(cig_color_ref, cig_vec2_t, cig_vec2_t, unsigned int);
void cig_set_draw_line_callback(cig_draw_line_callback_t);

/* ┌────────────────┐
───┤                │
   └────────────────┘ */
   
/* Fills current frame with the given panel style */
void cig_fill_panel(cig_panel_ref, cig_panel_modifiers_t);

/* */
void cig_fill_color(cig_color_ref);

void cig_draw_line(cig_color_ref, cig_vec2_t, cig_vec2_t, unsigned int);

#endif
