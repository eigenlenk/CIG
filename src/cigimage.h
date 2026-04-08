#ifndef CIG_IMAGE_INCLUDED
#define CIG_IMAGE_INCLUDED

#include "cigcore.h"

typedef void* cig_image_ref;

typedef enum M_PACKED {
  CIG_IMAGE_MODE_ASPECT_FIT = 0,
  CIG_IMAGE_MODE_ASPECT_FILL,
  CIG_IMAGE_MODE_SCALE_TO_FILL,
  CIG_IMAGE_MODE_CENTER,
  CIG_IMAGE_MODE_LEFT,
  CIG_IMAGE_MODE_RIGHT,
  CIG_IMAGE_MODE_TOP,
  CIG_IMAGE_MODE_BOTTOM,
  CIG_IMAGE_MODE_TOP_LEFT,
  CIG_IMAGE_MODE_TOP_RIGHT,
  CIG_IMAGE_MODE_BOTTOM_LEFT,
  CIG_IMAGE_MODE_BOTTOM_RIGHT
} cig_image_mode;

typedef cig_v (*cig_measure_image_callback)(cig_image_ref);
typedef void (*cig_draw_image_callback)(cig_buffer_ref, cig_r, cig_r, cig_image_ref, cig_image_mode);

/*  ┌───────────────────┐
    │ BACKEND CALLBACKS │
    └───────────────────┘ */

void cig_assign_measure_image(cig_measure_image_callback);

void cig_assign_draw_image(cig_draw_image_callback);

/*  ┌───────────────────────────┐
    │ IMAGE & 2D DRAW FUNCTIONS │
    └───────────────────────────┘ */

/*  Draws an image */
void cig_draw_image(cig_image_ref, cig_image_mode);

#endif
