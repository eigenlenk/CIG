#include "cigimage.h"
#include <math.h>

static cig_measure_image_callback measure_image = NULL;
static cig_draw_image_callback draw_image = NULL;

/*  ┌───────────────────┐
    │ BACKEND CALLBACKS │
    └───────────────────┘ */

void cig_assign_measure_image(cig_measure_image_callback fp) {
  measure_image = fp;
}

void cig_assign_draw_image(cig_draw_image_callback fp) {
  draw_image = fp;
}

/*  ┌───────────────────────────┐
    │ IMAGE & 2D DRAW FUNCTIONS │
    └───────────────────────────┘ */

void cig_draw_image(cig_image_ref image, cig_image_mode mode) {
  if (!draw_image || !measure_image) { /* Log an error? */ return; }

  const cig_r container = cig_r_inset(cig_absolute_rect(), cig_current()->insets);
  const cig_v size = measure_image(image);

  cig_r rect = container; /* Output image rect */

  switch (mode) {
  case CIG_IMAGE_MODE_ASPECT_FIT:
  case CIG_IMAGE_MODE_ASPECT_FILL:
    {
      const float src_aspect = size.x / size.y;
      const float dst_aspect = (float)container.w / container.h;
      const float scale = (mode == CIG_IMAGE_MODE_ASPECT_FIT ? (src_aspect > dst_aspect) : (src_aspect < dst_aspect))
          ? ((float)container.w / size.x)
          : ((float)container.h / size.y);
      const int scaled_w = round(size.x * scale);
      const int scaled_h = round(size.y * scale);

      rect = cig_r_make(
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

      rect = cig_r_make(
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
