#include "win95.h"

typedef enum {
  WINDOW_RESIZE_BOTTOM_RIGHT = 0,
  WINDOW_RESIZE_RIGHT,
  WINDOW_RESIZE_TOP_RIGHT,
  WINDOW_RESIZE_TOP,
  WINDOW_RESIZE_TOP_LEFT,
  WINDOW_RESIZE_LEFT,
  WINDOW_RESIZE_BOTTOM_LEFT,
  WINDOW_RESIZE_BOTTOM
} window_resize_edge_t;

static void handle_window_resize(window_t *, window_resize_edge_t);

bool window_begin(window_t *wnd, window_message_t *msg, bool *focused) {
  static struct {
    window_t *selected_window;
    bool active;
    cig_r original_rect;
  } window_drag = { 0 };

  cig_set_next_id(wnd->id);
  
  const cig_i wnd_insets = cig_i_uniform(wnd->flags & IS_RESIZABLE ? 4 : 3);

  if (!cig_push_frame_insets(wnd->rect, wnd_insets)) {
    return false;
  }

  cig_fill_panel(get_panel(PANEL_STANDARD_DIALOG), 0);

  *focused = cig_enable_focus();

  /* Titlebar */
  CIG_HSTACK(
    RECT_AUTO_H(18),
    CIG_INSETS(cig_i_uniform(2)),
    CIG_PARAMS({
      CIG_SPACING(2),
      CIG_ALIGNMENT_HORIZONTAL(CIG_LAYOUT_ALIGNS_RIGHT)
    })
  ) {
    cig_fill_solid(get_color(*focused ? COLOR_WINDOW_ACTIVE_TITLEBAR : COLOR_WINDOW_INACTIVE_TITLEBAR));
    cig_enable_interaction();

    /* TODO: This could probably be a little nicer to deal with */
    if (!window_drag.active && cig_pressed(CIG_INPUT_PRIMARY_ACTION, CIG_PRESS_INSIDE) && cig_input_state()->drag.active) {
      cig_input_state()->locked = true;
      window_drag.active = true;
      window_drag.original_rect = wnd->rect;
      window_drag.selected_window = wnd;
    } else if (window_drag.selected_window == wnd) {
      if (window_drag.active && cig_input_state()->drag.active) {
        wnd->rect = cig_r_make(
          CIG_CLAMP(window_drag.original_rect.x + cig_input_state()->drag.change.x, -(wnd->rect.w - 50), cig_layout_rect().w - 30),
          CIG_CLAMP(window_drag.original_rect.y + cig_input_state()->drag.change.y, 0, cig_layout_rect().h - 50),
          wnd->rect.w,
          wnd->rect.h
        );
      } else {
        window_drag.active = false;
      }
    }

    /*  This container is right-aligned, so x=0 will align the right edge to parent */
    if (icon_button(RECT_AUTO_W(16), IMAGE_CROSS)) {
      *msg = WINDOW_CLOSE;
    }

    CIG_HSTACK(_, CIG_PARAMS({
      CIG_SPACING(2)
    })) {
      if (wnd->icon >= 0) {
        CIG(RECT_AUTO_W(16)) {
          cig_draw_image(get_image(wnd->icon), CIG_IMAGE_MODE_CENTER);
        }
      }
      CIG(_) {
        cig_draw_label((cig_text_properties) {
          .font = get_font(FONT_BOLD),
          .color = *focused ? get_color(COLOR_WHITE) : get_color(COLOR_DIALOG_BACKGROUND),
          .alignment.horizontal = CIG_TEXT_ALIGN_LEFT,
          .max_lines = 1,
          .overflow = CIG_TEXT_SHOW_ELLIPSIS 
        }, wnd->title);
      }
    }
  }

  /*
   * Add resizable edge and corner regions.
   * Resetting insets temporarily makes adding the resize regions easier to calculate.
   */
  cig_current()->insets = cig_i_zero();

  if (wnd->flags & IS_RESIZABLE) {
    struct {
      cig_r rect;
      window_resize_edge_t edge;
    } resize_regions[10] = {
      { cig_r_make(CIG_W-4, 20, 4, CIG_H-(20+16)), WINDOW_RESIZE_RIGHT },
      { cig_r_make(CIG_W-4, 4, 4, 16), WINDOW_RESIZE_TOP_RIGHT },
      { cig_r_make(CIG_W-20, 0, 20, 4), WINDOW_RESIZE_TOP_RIGHT },
      { cig_r_make(20, 0, CIG_W-(20+20), 4), WINDOW_RESIZE_TOP },
      { cig_r_make(0, 4, 4, 16), WINDOW_RESIZE_TOP_LEFT },
      { cig_r_make(0, 0, 20, 4), WINDOW_RESIZE_TOP_LEFT },
      { cig_r_make(0, 20, 4, CIG_H-(20+16)), WINDOW_RESIZE_LEFT },
      { cig_r_make(0, CIG_H-20, 4, 16), WINDOW_RESIZE_BOTTOM_LEFT },
      { cig_r_make(0, CIG_H-4, 20, 4), WINDOW_RESIZE_BOTTOM_LEFT },
      { cig_r_make(20, CIG_H-4, CIG_W-(20+16), 4), WINDOW_RESIZE_BOTTOM },
    };

    int i;
    for (i = 0; i < 10; ++i) {
      CIG(resize_regions[i].rect) {
        cig_enable_interaction();
        handle_window_resize(wnd, resize_regions[i].edge);
      }
    }

    CIG(RECT(CIG_W_INSET-16, CIG_H_INSET-16, 16, 16)) {
      cig_enable_interaction();
      handle_window_resize(wnd, WINDOW_RESIZE_BOTTOM_RIGHT);
    }
  }

  cig_current()->insets = wnd_insets;

  cig_push_frame(cig_r_make(0, 20, CIG_AUTO(), CIG_H_INSET - 20));
    
  return true;
}

void window_end(window_t *wnd) {
  cig_pop_frame(); /* Content frame */
  cig_pop_frame(); /* Window panel */
}

/*
 * ┌──────────┐
 * │ INTERNAL │
 * └──────────┘
 */

static void handle_window_resize(window_t *wnd, window_resize_edge_t edge) {
  static struct {
    window_t *selected_window;
    window_resize_edge_t active_edge;
    bool active;
    cig_r original_rect;
  } window_resize = { 0 };

  const struct {
    int x, y, w, h;
  } edge_adjustments[8] = {
    { 0, 0, 1, 1 }, /* WINDOW_RESIZE_BOTTOM_RIGHT */
    { 0, 0, 1, 0 }, /* WINDOW_RESIZE_RIGHT */
    { 0, 1, 1, 0 }, /* WINDOW_RESIZE_TOP_RIGHT */
    { 0, 1, 0, 0 }, /* WINDOW_RESIZE_TOP */
    { 1, 1, 0, 0 }, /* WINDOW_RESIZE_TOP_LEFT */
    { 1, 0, 0, 0 }, /* WINDOW_RESIZE_LEFT */
    { 1, 0, 0, 1 }, /* WINDOW_RESIZE_BOTTOM_LEFT */
    { 0, 0, 0, 1 }, /* WINDOW_RESIZE_BOTTOM */
  };

  if (!window_resize.active && cig_focused_id() == wnd->id && cig_pressed(CIG_INPUT_PRIMARY_ACTION, CIG_PRESS_INSIDE) && cig_input_state()->drag.active) {
    cig_input_state()->locked = true;
    window_resize.active = true;
    window_resize.original_rect = wnd->rect;
    window_resize.selected_window = wnd;
    window_resize.active_edge = edge;
  } else if (window_resize.selected_window == wnd && window_resize.active_edge == edge) {
    if (window_resize.active && cig_input_state()->drag.active) {
      const int dx = cig_input_state()->drag.change.x,
                dy = cig_input_state()->drag.change.y;

      if (edge_adjustments[edge].x) {
        wnd->rect.w = CIG_MAX(wnd->min_size.x, window_resize.original_rect.w - dx);
        wnd->rect.x = window_resize.original_rect.x + (window_resize.original_rect.w - wnd->rect.w);
      }
      if (edge_adjustments[edge].y) {
        wnd->rect.h = CIG_MAX(wnd->min_size.y, window_resize.original_rect.h - dy);
        wnd->rect.y = window_resize.original_rect.y + (window_resize.original_rect.h - wnd->rect.h);
      }
      if (edge_adjustments[edge].w) {
        wnd->rect.w = CIG_MAX(wnd->min_size.x, window_resize.original_rect.w + dx);
      }
      if (edge_adjustments[edge].h) {
        wnd->rect.h = CIG_MAX(wnd->min_size.y, window_resize.original_rect.h + dy);
      }
    } else {
      window_resize.active = false;
    }
  }
}
