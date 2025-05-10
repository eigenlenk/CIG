#include "win95.h"
#include "apps/welcome/welcome.h"
#include "time.h"
#include "cigcorem.h"
#include <stdio.h>
#include <string.h>

#define TASKBAR_H 28

static win95_t *this = NULL;

static void process_apps();
static void process_windows();
static void close_application(application_t *);

static bool taskbar_button(
  cig_r rect,
  const char *title,
  int icon,
  bool selected
) {
  bool clicked = false;
  
  CIG(rect, CIG_INSETS(cig_i_make(4, 2, 4, 2))) {
    cig_enable_interaction();
    
    const bool pressed = cig_pressed(CIG_MOUSE_BUTTON_ANY, CIG_PRESS_INSIDE);
    clicked = cig_clicked(CIG_MOUSE_BUTTON_ANY, CIG_CLICK_DEFAULT_OPTIONS);

    cig_fill_panel(get_panel(PANEL_BUTTON), selected ? CIG_PANEL_SELECTED : (pressed ? CIG_PANEL_PRESSED : 0));
    
    CIG_HSTACK(
      _,
      CIG_INSETS(selected ? cig_i_make(0, 1, 0, -1) : cig_i_zero()),
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

static bool start_button(cig_r rect) {
  bool clicked = false;
  
  CIG(rect, CIG_INSETS(cig_i_make(4, 2, 4, 2))) {
    cig_enable_interaction();
    
    const bool pressed = cig_pressed(CIG_MOUSE_BUTTON_ANY, CIG_PRESS_INSIDE);
    
    cig_fill_panel(get_panel(PANEL_BUTTON), pressed ? CIG_PANEL_PRESSED : 0);
    
    CIG_HSTACK(
      _,
      CIG_INSETS(pressed ? cig_i_make(0, 1, 0, -1) : cig_i_zero()),
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

  CIG_VSTACK(RECT_AUTO, CIG_INSETS(cig_i_make(2, 2, 2, 0)), CIG_PARAMS({
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
          cig_r_make(CIG_SX+text_x_in_parent-1, CIG_SY-1, label->bounds.w+2, label->bounds.h+2),
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
  if (!cig_push_grid(RECT_AUTO, cig_i_zero(), (cig_layout_params_t) {
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
    window_t *primary_wnd;
    if ((app = win95_find_open_app("welcome"))) {
      if ((primary_wnd = window_manager_find_primary_window(&this->window_manager, app))) {
        window_manager_bring_to_front(&this->window_manager, primary_wnd->id);
      } else {
        window_manager_create(&this->window_manager, app, app->windows[0]);
      }
    } else {
      win95_open_app(welcome_app());
    }
  }

  cig_pop_frame();
}

static void do_desktop() {
  if (cig_push_frame(cig_r_make(0, 0, CIG_W, CIG_H - TASKBAR_H))) {
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
    cig_r_make(0, CIG_H - TASKBAR_H, CIG_W, TASKBAR_H),
    CIG_INSETS(cig_i_make(2, 4, 2, 2))
  ) {
    cig_fill_solid(get_color(COLOR_DIALOG_BACKGROUND));
    cig_draw_line(cig_v_make(CIG_SX, CIG_SY+1), cig_v_make(CIG_SX+CIG_W, CIG_SY+1), get_color(COLOR_WHITE), 1);

    /* Left: Start button */
    start_button(RECT_AUTO_W(start_button_width));
    
    /* Right: Calculate clock bounds */
    time_t t = time(NULL);
    struct tm *ct = localtime(&t);

    cig_prepare_label(&clock_label, 0, (cig_text_properties_t) { .flags = CIG_TEXT_FORMATTED }, "%02d:%02d", ct->tm_hour, ct->tm_min);

    const int clock_w = (clock_label.bounds.w+11*2);

    CIG(
      cig_r_make(CIG_W_INSET-clock_w, 0, clock_w, CIG_AUTO()),
      CIG_INSETS(cig_i_uniform(1))
    ) {
      cig_fill_panel(get_panel(PANEL_INNER_BEVEL_NO_FILL), 0);
      cig_draw_label(&clock_label);
    }

    /* Center: Fill remaining middle space with task buttons */
    CIG_HSTACK(
      cig_r_make(start_button_width+spacing, 0, CIG_W_INSET-start_button_width-clock_w-spacing*2, CIG_AUTO()),
      CIG_PARAMS({
        CIG_SPACING(spacing),
        CIG_COLUMNS(this->running_apps),
        CIG_MAX_WIDTH(150)
      })
    ) {
      for (register size_t i = 0; i < WIN95_OPEN_WINDOWS_MAX; ++i) {
        window_t *wnd = &this->window_manager.windows[i];
        if (wnd->id && taskbar_button(RECT_AUTO, wnd->title, wnd->icon, wnd->id == cig_focused_id())) {
          cig_set_focused_id(wnd->id);
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
  process_apps();
  process_windows();
  do_taskbar();
}

void win95_open_app(application_t app) {
  application_t *new_app = &this->applications[this->running_apps++];
  *new_app = app;
  window_manager_create(&this->window_manager, new_app, new_app->windows[0]);
}

application_t *win95_find_open_app(const char *id) {
  register size_t i;
  application_t *app;
  for (i = 0; i < this->running_apps; ++i) {
    app = &this->applications[i];
    if (!strcmp(app->id, id)) {
      return app;
    }
  }
  return NULL;
}

static void process_apps() {
  register size_t i;
  application_t *app;

  for (i = 0; i < this->running_apps; ++i) {
    app = &this->applications[i];

    if (app->proc) {
      /* App results not really implemented right now */
      app->proc(app);
      // switch (app->proc(app)) { }
    }
  }
}

static void process_windows() {
  /* Make sure focused window is the topmost one */
  if (this->window_manager.count > 0 && this->window_manager.order[this->window_manager.count-1]->id != cig_focused_id()) {
    window_manager_bring_to_front(&this->window_manager, cig_focused_id());
  }

  for (register size_t i = 0; i < this->window_manager.count;) {
    window_t *wnd = this->window_manager.order[i];

    if (wnd->proc) {
      switch (wnd->proc(wnd)) {
        case WINDOW_CLOSE: {
          window_manager_close(&this->window_manager, wnd);
          continue;
        };
        default: break;
      }
    }

    ++i;
  }
}

static void close_application(application_t *app) {
  register size_t i, j;
  app->proc = NULL;
  if (app->data) {
    free(app->data);
    app->data = NULL;
  }
  for (i = 0; i < this->running_apps; ++i) {
    if (&this->applications[i] == app) {
      for (j = i+1; j < this->running_apps; ++j) {
        this->applications[j-1] = this->applications[j];
      }
      this->running_apps--;
    }
  }
}

/*  ┌────────────────┐
    │ WINDOW MANAGER │
    └────────────────┘ */

window_t* window_manager_create(window_manager_t *manager, application_t *app, window_t wnd) {
  for (register size_t i = 0; i < WIN95_OPEN_WINDOWS_MAX; ++i) {
    if (manager->windows[i].id) {
      continue;
    }
    wnd.owner = app;
    wnd.id += rand() % 10000000;
    manager->windows[i] = wnd;
    manager->order[manager->count++] = &manager->windows[i];
    cig_set_focused_id(wnd.id);
    return manager->order[manager->count-1];
  }
  return NULL;
}

void window_manager_close(window_manager_t *manager, window_t *wnd) {
  register size_t i, j;
  if (wnd->owner && wnd->flags & IS_PRIMARY_WINDOW && wnd->owner->flags & KILL_WHEN_PRIMARY_WINDOW_CLOSED) {
    close_application(wnd->owner);
  }
  wnd->id = 0;
  for (i = 0; i < manager->count; ++i) {
    if (manager->order[i] == wnd) {
      for (j = i+1; j < manager->count; ++j) {
        manager->order[j-1] = manager->order[j];
      }
      break;
    }
  }
  manager->count--;
}

void window_manager_bring_to_front(window_manager_t *manager, cig_id_t wnd_id) {
  for (register size_t i = 0; i < manager->count; ++i) {
    if (manager->order[i]->id == wnd_id) {
      window_t *wnd = manager->order[i];
      manager->order[i] = manager->order[manager->count-1];
      manager->order[manager->count-1] = wnd;
      cig_set_focused_id(wnd_id);
      break;
    }
  }
}

window_t* window_manager_find_primary_window(window_manager_t *manager, application_t *app) {
  for (register size_t i = 0; i < manager->count; ++i) {
    if (manager->order[i]->owner == app && manager->order[i]->flags & IS_PRIMARY_WINDOW) {
      return manager->order[i];
    }
  }

  return NULL;
}

/*  ┌───────────────────┐
    │ COMMON COMPONENTS │
    └───────────────────┘ */

bool standard_button(cig_r rect, const char *title) {
  bool clicked = false;
  
  CIG(rect) {
    cig_enable_interaction();
    
    const bool pressed = cig_pressed(CIG_MOUSE_BUTTON_ANY, CIG_PRESS_INSIDE);
    
    cig_fill_panel(get_panel(PANEL_BUTTON), pressed ? CIG_PANEL_PRESSED : 0);
    
    if (cig_push_frame_insets(RECT_AUTO,  pressed ? cig_i_make(2, 3, 1, 2) : cig_i_make(1, 1, 2, 2))) {
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

bool icon_button(cig_r rect, image_id_t image_id) {
  bool clicked = false;
  
  CIG(rect) {
    cig_enable_interaction();
    
    const bool pressed = cig_pressed(CIG_MOUSE_BUTTON_ANY, CIG_PRESS_INSIDE);
    
    cig_fill_panel(get_panel(PANEL_BUTTON), pressed ? CIG_PANEL_PRESSED : 0);
    
    if (cig_push_frame_insets(RECT_AUTO, pressed ? cig_i_make(3, 3, 1, 1) : cig_i_make(2, 2, 2, 2))) {
      cig_image(get_image(image_id), CIG_IMAGE_MODE_CENTER);
      cig_pop_frame();
    }
    
    clicked = cig_clicked(CIG_MOUSE_BUTTON_ANY, CIG_CLICK_DEFAULT_OPTIONS);
  }
  
  return clicked;
}

bool checkbox(cig_r rect, bool *value, const char *text) {
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
        cig_draw_line(cig_v_make(CIG_SX+2, CIG_SY+1), cig_v_make(CIG_SX+11, CIG_SY+1), get_color(COLOR_BLACK), 1);
        cig_draw_line(cig_v_make(CIG_SX+2, CIG_SY+1), cig_v_make(CIG_SX+2, CIG_SY+11), get_color(COLOR_BLACK), 1);
        cig_draw_line(cig_v_make(CIG_SX+1, CIG_SY+11), cig_v_make(CIG_SX+12, CIG_SY+11), get_color(COLOR_DIALOG_BACKGROUND), 1);
        cig_draw_line(cig_v_make(CIG_SX+12, CIG_SY+11), cig_v_make(CIG_SX+12, CIG_SY+1), get_color(COLOR_DIALOG_BACKGROUND), 1);

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
    window_t *selected_window;
    bool active;
    cig_r original_rect;
  } window_drag = { 0 };

  window_message_t msg = 0;

  cig_set_next_id(wnd->id);
  /*  We're not checking the return value here, so if it's FALSE (= culled)
      it would crash. Easiest fix is to make sure the window can't be dragged
      off-screen all the way. */
  cig_push_frame_insets(wnd->rect, cig_i_uniform(3));
  cig_fill_panel(get_panel(PANEL_STANDARD_DIALOG), 0);

  const bool focused = cig_enable_focus();

  /* Titlebar */
  CIG_HSTACK(
    RECT_AUTO_H(18),
    CIG_INSETS(cig_i_uniform(2)),
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
      window_drag.selected_window = wnd;
    } else if (window_drag.selected_window == wnd) {
      if (window_drag.active && cig_input_state()->drag.active) {
        wnd->rect = cig_r_make(
          CIG_CLAMP(window_drag.original_rect.x + cig_input_state()->drag.change.x, -(wnd->rect.w - 50), 640 - 30),
          CIG_CLAMP(window_drag.original_rect.y + cig_input_state()->drag.change.y, 0, 480 - 50),
          wnd->rect.w,
          wnd->rect.h
        );
      } else {
        window_drag.active = false;
      }
    }

    cig_label((cig_text_properties_t) {
      .font = get_font(FONT_BOLD),
      .color = get_color(COLOR_WHITE),
      .alignment.horizontal = CIG_TEXT_ALIGN_LEFT,
      .alignment.vertical = CIG_TEXT_ALIGN_MIDDLE
    }, wnd->title);
    
    /*  This container is right-aligned, so X will be 0 */
    if (icon_button(RECT_AUTO_W(16), IMAGE_CROSS)) {
      msg = WINDOW_CLOSE;
    }
  }

  cig_push_frame(cig_r_make(0, 18, CIG_AUTO(), CIG_H_INSET - 18));
    
  return msg;
}

void end_window() {
  cig_pop_frame(); /* Content frame */
  cig_pop_frame(); /* Window panel */
}