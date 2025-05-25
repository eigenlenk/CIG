#ifndef CIG_GFX_INCLUDED
#define CIG_GFX_INCLUDED

#include "cigcore.h"

typedef void* cig_color_ref;
typedef void* cig_style_ref;
typedef void* cig_image_ref;

typedef enum CIG_PACKED {
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

/* These can be application-specific and you can use custom flags as well */
typedef enum CIG_PACKED {
  CIG_STYLE_APPLY_HOVER = CIG_BIT(0),
  CIG_STYLE_APPLY_PRESS = CIG_BIT(1),
  CIG_STYLE_APPLY_SELECTION = CIG_BIT(2),
  CIG_STYLE_APPLY_FOCUS = CIG_BIT(3),
} cig_style_modifiers;
/* TODO: Maybe pass to draw_image and fill_solid as well? */

typedef cig_v (*cig_measure_image_callback)(cig_image_ref);
typedef void (*cig_draw_image_callback)(cig_buffer_ref, cig_r, cig_r, cig_image_ref, cig_image_mode);
typedef void (*cig_draw_style_callback)(cig_style_ref, cig_r, cig_style_modifiers);
typedef void (*cig_draw_rectangle_callback)(cig_color_ref, cig_color_ref, cig_r, unsigned int);
typedef void (*cig_draw_line_callback)(cig_color_ref, cig_v, cig_v, float);

/*  ┌───────────────────┐
    │ BACKEND CALLBACKS │
    └───────────────────┘ */

void cig_assign_measure_image(cig_measure_image_callback);

void cig_assign_draw_image(cig_draw_image_callback);

void cig_assign_draw_style(cig_draw_style_callback);

void cig_assign_draw_rectangle(cig_draw_rectangle_callback);

void cig_assign_draw_line(cig_draw_line_callback);

/*  ┌───────────────────────────┐
    │ IMAGE & 2D DRAW FUNCTIONS │
    └───────────────────────────┘ */

/*  Draws an image */
void cig_draw_image(cig_image_ref, cig_image_mode);

/*  Fills current frame with the given style */
void cig_fill_style(cig_style_ref, cig_style_modifiers);

/*  Fills current frame with color */
void cig_fill_color(cig_color_ref);

void cig_draw_line(cig_v, cig_v, cig_color_ref, float);

void cig_draw_rect(cig_r, cig_color_ref, cig_color_ref, float);

#endif
