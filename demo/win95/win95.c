#include "win95.h"
#include "apps/welcome/welcome.h"
#include "time.h"
#include "cigcorem.h"
#include <stdio.h>
#include <string.h>

#define TASKBAR_H 28

static win95_t *this = NULL;

static void run_apps();
static void close_application(application_t *);
static void close_window(window_t *);

static bool taskbar_button(
  cig_rect_t rect,
  const char *title,
  int icon,
  bool selected
) {
  bool clicked = false;
  
  CIG(rect, CIG_INSETS(cig_insets_make(4, 2, 4, 2))) {
    cig_enable_interaction();
    
    const bool pressed = cig_pressed(CIG_MOUSE_BUTTON_ANY, CIG_PRESS_INSIDE);
    clicked = cig_clicked(CIG_MOUSE_BUTTON_ANY, CIG_CLICK_DEFAULT_OPTIONS);

    cig_fill_panel(get_panel(PANEL_BUTTON), selected ? CIG_PANEL_SELECTED : (pressed ? CIG_PANEL_PRESSED : 0));
    
    CIG_HSTACK(
      _,
      CIG_INSETS(selected ? cig_insets_make(0, 1, 0, -1) : cig_insets_zero()),
      CIG_PARAMS({
        CIG_SPACING(2)
      })
    ) {
      if (icon >= 0) {
        CIG(RECT_AUTO_W(16)) {
          cig_image(get_image(icon), CIG_IMAGE_MODE_CENTER);
        }
      }
      if (title) {
        CIG(_) {
          cig_label((cig_text_properties_t) {
            .font = get_font(selected ? FONT_BOLD : FONT_REGULAR),
            .alignment.horizontal = CIG_TEXT_ALIGN_LEFT,
            .overflow = CIG_TEXT_SHOW_ELLIPSIS
          }, title);
        }
      }
    }
  }
  
  return clicked;
}

static bool start_button(cig_rect_t rect) {
  bool clicked = false;
  
  CIG(rect, CIG_INSETS(cig_insets_make(4, 2, 4, 2))) {
    cig_enable_interaction();
    
    const bool pressed = cig_pressed(CIG_MOUSE_BUTTON_ANY, CIG_PRESS_INSIDE);
    
    cig_fill_panel(get_panel(PANEL_BUTTON), pressed ? CIG_PANEL_PRESSED : 0);
    
    CIG_HSTACK(
      _,
      CIG_INSETS(pressed ? cig_insets_make(0, 1, 0, -1) : cig_insets_zero()),
      CIG_PARAMS({
        CIG_SPACING(2)
      })
    ) {
      CIG(RECT_AUTO_W(16)) {
        cig_image(get_image(IMAGE_START_ICON), CIG_IMAGE_MODE_CENTER);
      }

      CIG(_) {
        cig_label((cig_text_properties_t) {
          .font = get_font(FONT_BOLD),
          .alignment.horizontal = CIG_TEXT_ALIGN_LEFT
        }, "Start");
      }
    }

    clicked = cig_clicked(CIG_MOUSE_BUTTON_ANY, CIG_CLICK_DEFAULT_OPTIONS);
  }
  
  return clicked;
}

static bool do_desktop_icon(int icon, const char *title) {
  bool double_clicked = false;

  CIG_VSTACK(RECT_AUTO, CIG_INSETS(cig_insets_make(2, 2, 2, 0)), CIG_PARAMS({
    CIG_SPACING(6)
  })) {
    cig_enable_interaction();

    const bool focused = cig_enable_focus();
    double_clicked = cig_clicked(CIG_MOUSE_BUTTON_ANY, CIG_CLICK_DEFAULT_OPTIONS | CIG_CLICK_DOUBLE);

    CIG(RECT_AUTO_H(32)) {
      if (focused) { enable_blue_selection_dithering(true); }
      cig_image(get_image(icon), CIG_IMAGE_MODE_TOP);
      if (focused) { enable_blue_selection_dithering(false); }
    }

    cig_label_t *label = CIG_ALLOCATE(cig_label_t);

    /* We need to prepare the label here to know how large of a rectangle
       to draw around it when the icon is focused */
    cig_prepare_label(label, CIG_W, (cig_text_properties_t) {
        .color = get_color(COLOR_WHITE),
        .alignment.vertical = CIG_TEXT_ALIGN_TOP
      }, title);

    CIG(_) {
      if (focused) {
        int text_x_in_parent = (CIG_W - label->bounds.w) * 0.5;
        cig_draw_rect(
          cig_rect_make(CIG_SX+text_x_in_parent-1, CIG_SY-1, label->bounds.w+2, label->bounds.h+2),
          get_color(COLOR_WINDOW_ACTIVE_TITLEBAR),
          0,
          1
        );
      }
      cig_draw_label(label);
    }
  }

  return double_clicked;
}

static void do_desktop_icons() {
  if (!cig_push_grid(RECT_AUTO, cig_insets_zero(), (cig_layout_params_t) {
    .width = 75,
    .height = 75,
    .direction = CIG_LAYOUT_DIRECTION_VERTICAL
  })) {
    return;
  }

  do_desktop_icon(IMAGE_MY_COMPUTER_32, "My Computer");
  do_desktop_icon(IMAGE_BIN_EMPTY, "Recycle Bin");

  if (do_desktop_icon(IMAGE_WELCOME_APP_ICON, "Welcome")) {
    application_t *app;
    if ((app = win95_find_open_app("welcome"))) {
      // todo: window manager -> bring to front (wnd)
      cig_set_focused_id(app->windows[0].id);
    } else {
      win95_open_app(welcome_app());
    }
  }

  cig_pop_frame();
}

static void do_desktop() {
  if (cig_push_frame(cig_rect_make(0, 0, CIG_W, CIG_H - TASKBAR_H))) {
    cig_fill_solid(get_color(COLOR_DESKTOP));
    cig_enable_focus();
    do_desktop_icons();
    cig_pop_frame();
  }
}

static void do_taskbar() {
  static cig_label_t clock_label;

  const int start_button_width = 54;
  const int spacing = 4;

  CIG(
    cig_rect_make(0, CIG_H - TASKBAR_H, CIG_W, TASKBAR_H),
    CIG_INSETS(cig_insets_make(2, 4, 2, 2))
  ) {
    cig_fill_solid(get_color(COLOR_DIALOG_BACKGROUND));
    cig_draw_line(cig_vec2_make(CIG_SX, CIG_SY+1), cig_vec2_make(CIG_SX+CIG_W, CIG_SY+1), get_color(COLOR_WHITE), 1);

    /* Left: Start button */
    start_button(RECT_AUTO_W(start_button_width));
    
    /* Right: Calculate clock bounds */
    time_t t = time(NULL);
    struct tm *ct = localtime(&t);

    cig_prepare_label(&clock_label, 0, (cig_text_properties_t) { .flags = CIG_TEXT_FORMATTED }, "%02d:%02d", ct->tm_hour, ct->tm_min);

    const int clock_w = (clock_label.bounds.w+11*2);

    CIG(
      cig_rect_make(CIG_W_INSET-clock_w, 0, clock_w, CIG_AUTO(0)),
      CIG_INSETS(cig_insets_uniform(1))
    ) {
      cig_fill_panel(get_panel(PANEL_INNER_BEVEL_NO_FILL), 0);
      cig_draw_label(&clock_label);
    }

    /* Center: Fill remaining middle space with task buttons */
    CIG_HSTACK(
      cig_rect_make(start_button_width+spacing, 0, CIG_W_INSET-start_button_width-clock_w-spacing*2, CIG_AUTO(0)),
      CIG_PARAMS({
        CIG_SPACING(spacing),
        CIG_COLUMNS(this->running_apps),
        CIG_MAX_WIDTH(150)
      })
    ) {
      register size_t i, j;
      window_t *wnd;

      for (i = 0; i < this->running_apps; ++i) {
        for (j = 0; j < 1; ++j) {
          wnd = &this->applications[i].windows[j];
          if (taskbar_button(RECT_AUTO, wnd->title, wnd->icon, wnd->id == cig_focused_id())) {
            // todo: window manager -> bring to front (wnd)
            cig_set_focused_id(wnd->id);
          }
        }
      }
    }
  }
}

void start_win95(win95_t *win95) {
  this = win95;

  win95_open_app(welcome_app());
}

void run_win95(win95_t *win95) {
  this = win95;

  do_desktop();
  run_apps();
  do_taskbar();
}

void win95_open_app(application_t app) {
  this->applications[this->running_apps++] = app;
  cig_set_focused_id(this->applications[this->running_apps-1].windows[0].id);
}

application_t *win95_find_open_app(const char *id) {
  register size_t i;
  application_t *app;
  for (i = 0; i < this->running_apps; ++i) {
    app = &this->applications[i];
    if (!app->closed && !strcmp(app->id, id)) {
      return app;
    }
  }
  return NULL;
}

static void run_apps() {
  register size_t i, j;
  application_t *app;
  window_t *wnd;

  for (i = 0; i < this->running_apps; ++i) {
    app = &this->applications[i];

    if (app->proc) {
      switch (app->proc(app)) {
        case APPLICATION_KILL: {
          for (j = 0; j < 1; ++j) {
            close_window(&app->windows[j]);
          }
          close_application(app);
          continue;
        }
        default: break;
      }
    }

    for (j = 0; j < 1; ++j) {
      wnd = &app->windows[j];

      if (wnd->proc) {
        switch (wnd->proc(wnd)) {
          case WINDOW_CLOSE: {
            close_window(wnd);
            close_application(app);
            
          } break;
          default: break;
        }
      }
    }
  }

  /* Remove closed apps */
  for (i = 0; i < this->running_apps; ++i) {
    if (this->applications[i].closed) {
      for (j = i+1; j < this->running_apps; ++j) {
        this->applications[j-1] = this->applications[j];
      }
      this->running_apps--;
    }
  }
}

static void close_application(application_t *app) {
  app->proc = NULL;
  app->closed = true;
  if (app->data) {
    free(app->data);
    app->data = NULL;
  }
}

static void close_window(window_t *wnd) {
  wnd->proc = NULL;
  wnd->title = NULL;
  if (wnd->data) {
    free(wnd->data);
    wnd->data = NULL;
  }
}

/* Application */

void application_open_window(application_t *app, window_t wnd) {
  app->windows[0] = wnd;
}

/* Components */

bool standard_button(cig_rect_t rect, const char *title) {
  bool clicked = false;
  
  CIG(rect) {
    cig_enable_interaction();
    
    const bool pressed = cig_pressed(CIG_MOUSE_BUTTON_ANY, CIG_PRESS_INSIDE);
    
    cig_fill_panel(get_panel(PANEL_BUTTON), pressed ? CIG_PANEL_PRESSED : 0);
    
    if (cig_push_frame_insets(RECT_AUTO,  pressed ? cig_insets_make(2, 3, 1, 2) : cig_insets_make(1, 1, 2, 2))) {
      cig_label((cig_text_properties_t) {
        .font = get_font(FONT_REGULAR),
        .max_lines = 1,
        .overflow = CIG_TEXT_SHOW_ELLIPSIS
      }, title);
      cig_pop_frame();
    }
    
    clicked = cig_clicked(CIG_MOUSE_BUTTON_ANY, CIG_CLICK_DEFAULT_OPTIONS);
  }
  
  return clicked;
}

bool icon_button(cig_rect_t rect, image_id_t image_id) {
  bool clicked = false;
  
  CIG(rect) {
    cig_enable_interaction();
    
    const bool pressed = cig_pressed(CIG_MOUSE_BUTTON_ANY, CIG_PRESS_INSIDE);
    
    cig_fill_panel(get_panel(PANEL_BUTTON), pressed ? CIG_PANEL_PRESSED : 0);
    
    if (cig_push_frame_insets(RECT_AUTO, pressed ? cig_insets_make(3, 3, 1, 1) : cig_insets_make(2, 2, 2, 2))) {
      cig_image(get_image(image_id), CIG_IMAGE_MODE_CENTER);
      cig_pop_frame();
    }
    
    clicked = cig_clicked(CIG_MOUSE_BUTTON_ANY, CIG_CLICK_DEFAULT_OPTIONS);
  }
  
  return clicked;
}

bool checkbox(cig_rect_t rect, bool *value, const char *text) {
  bool toggled = false;
  CIG_HSTACK(
    rect,
    CIG_PARAMS({
      CIG_SPACING(5)
    })
  ) {
    cig_enable_interaction();

    if (!value) {
      value = CIG_ALLOCATE(bool);
    }

    const bool pressed = cig_pressed(CIG_MOUSE_BUTTON_ANY, CIG_PRESS_DEFAULT_OPTIONS);

    if (cig_clicked(CIG_MOUSE_BUTTON_ANY, CIG_CLICK_DEFAULT_OPTIONS)) {
      *value = !*value;
    }

    CIG(RECT_AUTO_W(13)) {
      CIG(RECT_CENTERED_VERTICALLY(RECT_SIZED(13, 13))) {
        cig_fill_solid(pressed ? get_color(COLOR_DIALOG_BACKGROUND) : get_color(COLOR_WHITE));
        cig_fill_panel(get_panel(PANEL_INNER_BEVEL_NO_FILL), 0);
        cig_draw_line(cig_vec2_make(CIG_SX+2, CIG_SY+1), cig_vec2_make(CIG_SX+11, CIG_SY+1), get_color(COLOR_BLACK), 1);
        cig_draw_line(cig_vec2_make(CIG_SX+2, CIG_SY+1), cig_vec2_make(CIG_SX+2, CIG_SY+11), get_color(COLOR_BLACK), 1);
        cig_draw_line(cig_vec2_make(CIG_SX+1, CIG_SY+11), cig_vec2_make(CIG_SX+12, CIG_SY+11), get_color(COLOR_DIALOG_BACKGROUND), 1);
        cig_draw_line(cig_vec2_make(CIG_SX+12, CIG_SY+11), cig_vec2_make(CIG_SX+12, CIG_SY+1), get_color(COLOR_DIALOG_BACKGROUND), 1);

        if (*value) {
          cig_image(get_image(IMAGE_CHECKMARK), CIG_IMAGE_MODE_CENTER);
        }
      }
    }
    CIG(_) {
      cig_label((cig_text_properties_t) {
        .alignment.horizontal = CIG_TEXT_ALIGN_LEFT
      }, text);
    }
  }
  return toggled;
}

window_message_t begin_window(window_t *wnd) {
  static struct {
    bool active;
    cig_rect_t original_rect;
  } window_drag = { 0 };

  window_message_t msg = 0;

  cig_set_next_id(wnd->id);
  /*  We're not checking the return value here, so if it's FALSE (= culled)
      it would crash. Easiest fix is to make sure the window can't be dragged
      off-screen all the way. */
  cig_push_frame_insets(wnd->rect, cig_insets_uniform(3));
  cig_fill_panel(get_panel(PANEL_STANDARD_DIALOG), 0);

  const bool focused = cig_enable_focus();

  /* Titlebar */
  CIG_HSTACK(
    RECT_AUTO_H(18),
    CIG_INSETS(cig_insets_uniform(2)),
    CIG_PARAMS({
      CIG_SPACING(2),
      CIG_ALIGNMENT_HORIZONTAL(CIG_LAYOUT_ALIGNS_RIGHT)
    })
  ) {
    cig_fill_solid(get_color(focused ? COLOR_WINDOW_ACTIVE_TITLEBAR : COLOR_WINDOW_INACTIVE_TITLEBAR));
    cig_enable_interaction();

    /* TODO: This could probably be a little nicer to deal with */
    if (!window_drag.active && cig_pressed(CIG_INPUT_MOUSE_BUTTON_LEFT, CIG_PRESS_INSIDE) && cig_input_state()->drag.active) {
      cig_input_state()->locked = true;
      window_drag.active = true;
      window_drag.original_rect = wnd->rect;
    } else if (window_drag.active && cig_input_state()->drag.active) {
      wnd->rect = cig_rect_make(
        CIG_CLAMP(window_drag.original_rect.x + cig_input_state()->drag.change.x, -(wnd->rect.w - 50), 640 - 30),
        CIG_CLAMP(window_drag.original_rect.y + cig_input_state()->drag.change.y, 0, 480 - 50),
        wnd->rect.w,
        wnd->rect.h
      );
    } else {
      window_drag.active = false;
    }

    cig_label((cig_text_properties_t) {
      .font = get_font(FONT_BOLD),
      .color = get_color(COLOR_WHITE),
      .alignment.horizontal = CIG_TEXT_ALIGN_LEFT,
      .alignment.vertical = CIG_TEXT_ALIGN_MIDDLE
    }, wnd->title);
    
    if (icon_button(cig_rect_make(CIG_W_INSET - 16, 0, 16, CIG_AUTO(0)), IMAGE_CROSS)) {
      msg = WINDOW_CLOSE;
    }
  }

  cig_push_layout_function(
    &cig_default_layout_builder,
    cig_rect_make(0, 18, CIG_AUTO(0), CIG_H_INSET - 18),
    cig_insets_zero(),
    (cig_layout_params_t) { 0 }
  );
    
  return msg;
}

void end_window() {
  cig_pop_frame(); /* Content frame */
  cig_pop_frame(); /* Window panel */
}