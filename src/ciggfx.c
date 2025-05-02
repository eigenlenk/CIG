#include "ciggfx.h"

static cig_panel_render_callback_t panel_callback = NULL;
static cig_draw_rectangle_callback_t draw_rectangle = NULL;
static cig_draw_line_callback_t draw_line = NULL;
static cig_draw_image_callback_t draw_image = NULL;

/*  ┌───────────────────┐
    │ BACKEND CALLBACKS │
    └───────────────────┘ */

void cig_set_draw_image_callback(cig_draw_image_callback_t f) {
  draw_image = f;
}

void cig_set_panel_render_callback(cig_panel_render_callback_t f) {
  panel_callback = f;
}

void cig_set_draw_rectangle_callback(cig_draw_rectangle_callback_t f) {
  draw_rectangle = f;
}

void cig_set_draw_line_callback(cig_draw_line_callback_t f) {
  draw_line = f;
}

/*  ┌───────────────────────────┐
    │ IMAGE & 2D DRAW FUNCTIONS │
    └───────────────────────────┘ */

void cig_image(cig_image_ref image, cig_image_mode_t mode) {
  if (!draw_image) { /* Log an error? */ return; }
  draw_image(cig_buffer(), cig_rect_inset(cig_absolute_rect(), cig_frame()->insets), image, mode);
#ifdef DEBUG
  cig_trigger_debug_stepper_breakpoint(cig_rect_inset(cig_absolute_rect(), cig_frame()->insets), cig_rect_zero());
#endif
}

void cig_fill_panel(cig_panel_ref panel, cig_panel_modifiers_t modifiers) {
  if (!panel_callback) { /* Log an error? */ return; }
  panel_callback(panel, cig_absolute_rect(), modifiers);
}

void cig_fill_solid(cig_color_ref color) {
  if (!draw_rectangle) { /* Log an error? */ return; }
  draw_rectangle(color, 0, cig_absolute_rect(), 0);
}

void cig_draw_line(cig_vec2_t p0, cig_vec2_t p1, cig_color_ref color, float thickness) {
  if (!draw_line) { /* Log an error? */ return; }
  draw_line(color, p0, p1, thickness);
}

void cig_draw_rect(cig_rect_t rect, cig_color_ref fill_color, cig_color_ref outline_color, float thickness) {
  if (!draw_rectangle) { /* Log an error? */ return; }
  draw_rectangle(fill_color, outline_color, rect, thickness);
}