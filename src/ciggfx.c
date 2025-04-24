#include "ciggfx.h"

static cig_panel_render_callback_t panel_callback = NULL;
static cig_draw_rectangle_callback_t draw_rectangle = NULL;
static cig_draw_line_callback_t draw_line = NULL;

void cig_set_panel_render_callback(cig_panel_render_callback_t f) {
  panel_callback = f;
}

void cig_set_draw_rectangle_callback(cig_draw_rectangle_callback_t f) {
  draw_rectangle = f;
}

void cig_set_draw_line_callback(cig_draw_line_callback_t f) {
  draw_line = f;
}

void cig_fill_panel(cig_panel_ref panel, cig_panel_modifiers_t modifiers) {
  if (!panel_callback) { /* Log an error? */  return; }
  
  panel_callback(panel, cig_absolute_rect(), modifiers);
}

void cig_fill_color(cig_color_ref color) {
  if (!draw_rectangle) { /* Log an error? */  return; }
  
  draw_rectangle(color, 0, cig_absolute_rect(), 0);
}

void cig_draw_line(cig_color_ref color, cig_vec2_t p0, cig_vec2_t p1, unsigned int thickness) {
  if (!draw_line) { /* Log an error? */  return; }
  
  register const cig_vec2_t origin = cig_vec2_make(cig_absolute_rect().x, cig_absolute_rect().y);
  draw_line(color, cig_vec2_add(origin, p0), cig_vec2_add(origin, p1), thickness);
}