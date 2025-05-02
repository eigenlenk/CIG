#include "ciggfx.h"

static cig_measure_image_callback_t measure_image = NULL;
static cig_panel_render_callback_t panel_callback = NULL;
static cig_draw_rectangle_callback_t draw_rectangle = NULL;
static cig_draw_line_callback_t draw_line = NULL;
static cig_draw_image_callback_t draw_image = NULL;

/*  ┌───────────────────┐
    │ BACKEND CALLBACKS │
    └───────────────────┘ */

void cig_set_measure_image_callback(cig_measure_image_callback_t fp) {
  measure_image = fp;
}

void cig_set_draw_image_callback(cig_draw_image_callback_t fp) {
  draw_image = fp;
}

void cig_set_panel_render_callback(cig_panel_render_callback_t fp) {
  panel_callback = fp;
}

void cig_set_draw_rectangle_callback(cig_draw_rectangle_callback_t fp) {
  draw_rectangle = fp;
}

void cig_set_draw_line_callback(cig_draw_line_callback_t fp) {
  draw_line = fp;
}

/*  ┌───────────────────────────┐
    │ IMAGE & 2D DRAW FUNCTIONS │
    └───────────────────────────┘ */

void cig_image(cig_image_ref image, cig_image_mode_t mode) {
  if (!draw_image || !measure_image) { /* Log an error? */ return; }

  const cig_rect_t container = cig_rect_inset(cig_absolute_rect(), cig_frame()->insets);
  const cig_vec2_t size = measure_image(image);

  cig_rect_t rect = container; /* Output image rect */

  switch (mode) {
  case CIG_IMAGE_MODE_ASPECT_FIT:
  case CIG_IMAGE_MODE_ASPECT_FILL:
    {
      const float src_aspect = size.x / size.y;
      const float dst_aspect = (float)container.w / container.h;
      const float scale = (mode == CIG_IMAGE_MODE_ASPECT_FIT ? (src_aspect > dst_aspect) : (src_aspect < dst_aspect))
          ? ((float)container.w / size.x)
          : ((float)container.h / size.y);
      const int scaled_h = round(size.y * scale);

      rect = cig_rect_make(
        container.x + (container.w - scaled_w) * 0.5,
        container.y + (container.h - scaled_h) * 0.5,
        scaled_w,
        scaled_h
      );
    } break;

  case CIG_IMAGE_MODE_SCALE_TO_FILL:
    /*  Container and final image rectangle are the same */
    break;

  default:
    {
      double positions[9][2] = {
        { 0.5, 0.5 }, /* CIG_IMAGE_MODE_CENTER */
        { 0.0, 0.5 }, /* CIG_IMAGE_MODE_LEFT */
        { 1.0, 0.5 }, /* CIG_IMAGE_MODE_RIGHT */
        { 0.5, 0.0 }, /* CIG_IMAGE_MODE_TOP */
        { 0.5, 1.0 }, /* CIG_IMAGE_MODE_BOTTOM */
        { 0.0, 0.0 }, /* CIG_IMAGE_MODE_TOP_LEFT */
        { 1.0, 0.0 }, /* CIG_IMAGE_MODE_TOP_RIGHT */
        { 0.0, 1.0 }, /* CIG_IMAGE_MODE_BOTTOM_LEFT */
        { 1.0, 1.0 }, /* CIG_IMAGE_MODE_BOTTOM_RIGHT */
      };

      rect = cig_rect_make(
        container.x + (container.w - size.x) * positions[mode-CIG_IMAGE_MODE_CENTER][0],
        container.y + (container.h - size.y) * positions[mode-CIG_IMAGE_MODE_CENTER][1],
        size.x,
        size.y
      );
    } break;
  }

  draw_image(cig_buffer(), container, rect, image, mode);

#ifdef DEBUG
  cig_trigger_layout_breakpoint(container, rect);
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