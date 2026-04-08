#ifndef CIG_GFX_EXTENSIONS_INCLUDED
#define CIG_GFX_EXTENSIONS_INCLUDED

#include "cigcore.h"

/**
 * Includes graphical extensions you can call to draw rectangles, lines and
 * fill with panels of certain style. This follows CIG conventions but is
 * not part of the library itself. It's just an example, and you can avoid
 * opaque color and style references if you hardcode to a particular backend.
 */

typedef void* cig_color_ref;
typedef void* cig_style_ref;

typedef enum M_PACKED {
  CIG_STYLE_APPLY_HOVER = M_BIT(0),
  CIG_STYLE_APPLY_PRESS = M_BIT(1),
  CIG_STYLE_APPLY_SELECTION = M_BIT(2),
  CIG_STYLE_APPLY_FOCUS = M_BIT(3),
} cig_style_modifiers;

typedef void (*cig_draw_style_callback)(cig_style_ref, cig_r, cig_style_modifiers);
typedef void (*cig_draw_rectangle_callback)(cig_color_ref, cig_color_ref, cig_r, unsigned int);
typedef void (*cig_draw_line_callback)(cig_color_ref, cig_v, cig_v, float);

/*  ┌───────────────────┐
    │ BACKEND CALLBACKS │
    └───────────────────┘ */

void cig_assign_draw_style(cig_draw_style_callback);

void cig_assign_draw_rectangle(cig_draw_rectangle_callback);

void cig_assign_draw_line(cig_draw_line_callback);

/*  ┌───────────────────┐
    │ 2D DRAW FUNCTIONS │
    └───────────────────┘ */

/*  Fills current frame with the given style */
void cig_fill_style(cig_style_ref, cig_style_modifiers);

/*  Fills current frame with color */
void cig_fill_color(cig_color_ref);

void cig_draw_line(cig_v, cig_v, cig_color_ref, float);

void cig_draw_rect(cig_r, cig_color_ref, cig_color_ref, float);

#endif
