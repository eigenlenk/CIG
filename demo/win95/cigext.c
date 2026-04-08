#include "cigext.h"

static cig_draw_style_callback style_callback = NULL;
static cig_draw_rectangle_callback draw_rectangle = NULL;
static cig_draw_line_callback draw_line = NULL;

/*  ┌───────────────────┐
    │ BACKEND CALLBACKS │
    └───────────────────┘ */

void cig_assign_draw_style(cig_draw_style_callback fp) {
  style_callback = fp;
}

void cig_assign_draw_rectangle(cig_draw_rectangle_callback fp) {
  draw_rectangle = fp;
}

void cig_assign_draw_line(cig_draw_line_callback fp) {
  draw_line = fp;
}

/*  ┌───────────────────┐
    │ 2D DRAW FUNCTIONS │
    └───────────────────┘ */

void cig_fill_style(cig_style_ref style, cig_style_modifiers modifiers) {
  if (!style_callback) { /* Log an error? */ return; }
  style_callback(style, cig_absolute_rect(), modifiers);
}

void cig_fill_color(cig_color_ref color) {
  if (!draw_rectangle) { /* Log an error? */ return; }
  draw_rectangle(color, 0, cig_absolute_rect(), 0);
}

void cig_draw_line(cig_v p0, cig_v p1, cig_color_ref color, float thickness) {
  if (!draw_line) { /* Log an error? */ return; }
  draw_line(color, p0, p1, thickness);
}

void cig_draw_rect(cig_r rect, cig_color_ref fill_color, cig_color_ref outline_color, float thickness) {
  if (!draw_rectangle) { /* Log an error? */ return; }
  draw_rectangle(fill_color, outline_color, rect, thickness);
}
