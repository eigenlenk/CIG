#include "win95.h"
#include "apps/welcome/welcome.h"
#include "time.h"
#include "cigcorem.h"
#include <stdio.h>

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

    clicked = cig_clicked(CIG_MOUSE_BUTTON_ANY, CIG_CLICK_DEFAULT_OPTIONS);
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

    double_clicked = cig_clicked(CIG_MOUSE_BUTTON_ANY, CIG_CLICK_DEFAULT_OPTIONS | CIG_CLICK_DOUBLE);

    CIG(RECT_AUTO_H(32)) {
      cig_image(get_image(icon), CIG_IMAGE_MODE_TOP);
    }
    CIG(_) {
      cig_label((cig_text_properties_t) {
        .color = get_color(COLOR_WHITE),
        .alignment.vertical = CIG_TEXT_ALIGN_TOP
      }, title);
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

  enable_blue_selection_dithering(true);
  if (do_desktop_icon(IMAGE_MY_COMPUTER_32, "My Computer")) {
    printf("Open 'My Computer'\n");
  }
  enable_blue_selection_dithering(false);
  
  do_desktop_icon(IMAGE_BIN_EMPTY, "Recycle Bin");
  do_desktop_icon(IMAGE_WELCOME_APP_ICON, "Welcome");

  cig_pop_frame();
}

static void do_desktop() {
  if (cig_push_frame(cig_rect_make(0, 0, CIG_W, CIG_H - TASKBAR_H))) {
    cig_fill_color(get_color(COLOR_DESKTOP));
    do_desktop_icons();
    cig_pop_frame();
  }
}

static void do_taskbar() {
  static label_t some_label, clock_label;
  static unsigned long cnt = 0;

  const int start_button_width = 54;
  const int spacing = 4;

  CIG(
    cig_rect_make(0, CIG_H - TASKBAR_H, CIG_W, TASKBAR_H),
    CIG_INSETS(cig_insets_make(2, 4, 2, 2))
  ) {
    cig_fill_color(get_color(COLOR_DIALOG_BACKGROUND));
    cig_draw_line(get_color(COLOR_WHITE), cig_vec2_make(0, 1), cig_vec2_make(CIG_W, 1), 1);

    /* Left: Start button */
    start_button(RECT_AUTO_W(start_button_width));
    
    /* Right: Calculate clock bounds */
    time_t t = time(NULL);
    struct tm *ct = localtime(&t);

    cig_prepare_label(&clock_label, (cig_text_properties_t) { .flags = CIG_TEXT_FORMATTED }, 0, "%02d:%02d", ct->tm_hour, ct->tm_min);

    const int clock_w = (clock_label.bounds.w+11*2);

    CIG(
      cig_rect_make(CIG_W_INSET-clock_w, 0, clock_w, CIG_AUTO_BIT),
      CIG_INSETS(cig_insets_uniform(1))
    ) {
      cig_fill_panel(get_panel(PANEL_INNER_BEVEL_NO_FILL), 0);
      cig_prepared_label(&clock_label);
    }

    /* Center: Fill remaining middle space with task buttons */

    CIG_HSTACK(
      cig_rect_make(start_button_width+spacing, 0, CIG_W_INSET-start_button_width-clock_w-spacing*2, CIG_AUTO_BIT),
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
          taskbar_button(RECT_AUTO, wnd->title, wnd->icon, wnd == this->selected_window);
        }
      }

      /*cig_prepare_label(&some_label, (cig_text_properties_t) { .flags = CIG_TEXT_FORMATTED }, 0, "Counter: %lu", cnt=cnt+(1+cnt*0.001));

      CIG({
        CIG_RECT(RECT_AUTO_W(some_label.bounds.w))
      }) {
        cig_prepared_label(&some_label);
      }*/
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
  this->selected_window = &this->applications[this->running_apps-1].windows[0];
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
            if (this->selected_window == wnd) {
              // TODO: select new active window
            }
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
        cig_fill_color(pressed ? get_color(COLOR_DIALOG_BACKGROUND) : get_color(COLOR_WHITE));
        cig_fill_panel(get_panel(PANEL_INNER_BEVEL_NO_FILL), 0);
        cig_draw_line(get_color(COLOR_BLACK), cig_vec2_make(2, 1), cig_vec2_make(11, 1), 1);
        cig_draw_line(get_color(COLOR_BLACK), cig_vec2_make(2, 1), cig_vec2_make(2, 11), 1);
        cig_draw_line(get_color(COLOR_DIALOG_BACKGROUND), cig_vec2_make(1, 11), cig_vec2_make(12, 11), 1);
        cig_draw_line(get_color(COLOR_DIALOG_BACKGROUND), cig_vec2_make(12, 11), cig_vec2_make(12, 1), 1);

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
  window_message_t msg = 0;

  cig_push_frame_insets(wnd->rect, cig_insets_uniform(3));
  cig_fill_panel(get_panel(PANEL_STANDARD_DIALOG), 0);

  /* Titlebar */
  CIG_HSTACK(
    RECT_AUTO_H(18),
    CIG_INSETS(cig_insets_uniform(2)),
    CIG_PARAMS({
      CIG_SPACING(2),
      CIG_ALIGNMENT_HORIZONTAL(CIG_LAYOUT_ALIGNS_RIGHT)
    })
  ) {
    cig_fill_color(get_color(wnd == this->selected_window ? COLOR_WINDOW_ACTIVE_TITLEBAR : COLOR_DESKTOP));

    cig_label((cig_text_properties_t) {
      .font = get_font(FONT_BOLD),
      .color = get_color(COLOR_WHITE),
      .alignment.horizontal = CIG_TEXT_ALIGN_LEFT,
      .alignment.vertical = CIG_TEXT_ALIGN_MIDDLE
    }, wnd->title);
    
    if (standard_button(cig_rect_make(CIG_W_INSET - 16, 0, 16, CIG_AUTO_BIT), "X")) {
      msg = WINDOW_CLOSE;
    }
  }

  cig_push_layout_function(
    &cig_default_layout_builder,
    cig_rect_make(0, 18, CIG_AUTO_BIT, CIG_H_INSET - 18),
    cig_insets_zero(),
    (cig_layout_params_t) { 0 }
  );
    
  return msg;
}

void end_window() {
  cig_pop_frame(); /* Content frame */
  cig_pop_frame(); /* Window panel */
}