#include "win95.h"
#include "apps/explorer/explorer.h"
#include "apps/welcome/welcome.h"
#include "cigcorem.h"
#include <time.h>
#include <stdio.h>
#include <string.h>

#define TASKBAR_H 28

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

static win95_t *this = NULL;

static void process_apps();
static void process_windows();
static void close_application(application_t *);
static bool begin_window(window_t*, window_message_t*, bool*);
static void end_window(window_t*);
static cig_frame* large_file_icon(int, const char*, color_id_t, bool, bool*, bool*);
static void open_explorer_at(const char*);
static void handle_window_resize(window_t*, window_resize_edge_t);
static cig_r rect_of(cig_v, cig_v);

static bool taskbar_button(
  cig_r rect,
  const char *title,
  int icon,
  bool selected
) {
  bool clicked = false;
  
  CIG(rect, CIG_INSETS(cig_i_make(4, 2, 4, 2))) {
    cig_enable_interaction();
    
    const bool pressed = cig_pressed(CIG_INPUT_PRIMARY_ACTION, CIG_PRESS_INSIDE);
    clicked = cig_clicked(CIG_INPUT_PRIMARY_ACTION, CIG_CLICK_DEFAULT_OPTIONS);

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
          cig_draw_image(get_image(icon), CIG_IMAGE_MODE_CENTER);
        }
      }
      if (title) {
        CIG(_) {
          cig_draw_label((cig_text_properties) {
            .font = get_font(selected ? FONT_BOLD : FONT_REGULAR),
            .alignment.horizontal = CIG_TEXT_ALIGN_LEFT,
            .max_lines = 1,
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
    
    const bool pressed = cig_pressed(CIG_INPUT_PRIMARY_ACTION, CIG_PRESS_INSIDE);
    
    cig_fill_panel(get_panel(PANEL_BUTTON), pressed ? CIG_PANEL_PRESSED : 0);
    
    CIG_HSTACK(
      _,
      CIG_INSETS(pressed ? cig_i_make(0, 1, 0, -1) : cig_i_zero()),
      CIG_PARAMS({
        CIG_SPACING(2)
      })
    ) {
      CIG(RECT_AUTO_W(16)) {
        cig_draw_image(get_image(IMAGE_START_ICON), CIG_IMAGE_MODE_CENTER);
      }

      CIG(_) {
        cig_draw_label((cig_text_properties) {
          .font = get_font(FONT_BOLD),
          .alignment.horizontal = CIG_TEXT_ALIGN_LEFT
        }, "Start");
      }
    }

    clicked = cig_clicked(CIG_INPUT_PRIMARY_ACTION, CIG_CLICK_DEFAULT_OPTIONS);
  }
  
  return clicked;
}

static void do_desktop_icons() {
  if (!begin_file_browser(RECT_AUTO, CIG_LAYOUT_DIRECTION_VERTICAL, COLOR_WHITE, cig_focused(), NULL)) {
    return;
  }

  if (file_item(IMAGE_MY_COMPUTER_32, "My Computer")) {
    open_explorer_at(explorer_path_my_computer);
  }
  
  if (file_item(IMAGE_BIN_EMPTY, "Recycle Bin")) {
    open_explorer_at(explorer_path_recycle_bin);
  }

  if (file_item(IMAGE_WELCOME_APP_ICON, "Welcome")) {
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

  end_file_browser();
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
  static struct {
    cig_label label;
    cig_span spans[1];
  } clock_st = { .label.spans = clock_st.spans };

  const int start_button_width = 54;
  const int spacing = 4;
  register size_t i;

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

    cig_label_prepare(&clock_st.label, cig_v_zero(), (cig_text_properties) { .flags = CIG_TEXT_FORMATTED }, "%02d:%02d", ct->tm_hour, ct->tm_min);

    const int clock_w = (clock_st.label.bounds.w+11*2);

    CIG(
      cig_r_make(CIG_W_INSET-clock_w, 0, clock_w, CIG_AUTO()),
      CIG_INSETS(cig_i_uniform(1))
    ) {
      cig_fill_panel(get_panel(PANEL_INNER_BEVEL_NO_FILL), 0);
      cig_label_draw(&clock_st.label);
    }

    /* Center: Fill remaining middle space with task buttons */
    CIG_HSTACK(
      cig_r_make(start_button_width+spacing, 0, CIG_W_INSET-start_button_width-clock_w-spacing*2, CIG_AUTO()),
      CIG_PARAMS({
        CIG_SPACING(spacing),
        CIG_COLUMNS(this->window_manager.count),
        CIG_MAX_WIDTH(150)
      })
    ) {
      for (i = 0; i < this->window_manager.count; ++i) {
        window_t *wnd = &this->window_manager.windows[i];
        if (taskbar_button(RECT_AUTO, wnd->title, wnd->icon, wnd->id == cig_focused_id())) {
          cig_set_focused_id(wnd->id);
        }
      }
    }
  }
}

void start_win95(win95_t *win95) {
  this = win95;

  win95_open_app(explorer_app());
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
  if (new_app->windows[0].id) {
    window_manager_create(&this->window_manager, new_app, new_app->windows[0]);
  }
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

/*  ┌────────────────┐
    │ WINDOW MANAGER │
    └────────────────┘ */

window_t* window_manager_create(window_manager_t *manager, application_t *app, window_t wnd) {
  register size_t i;
  for (i = 0; i < WIN95_OPEN_WINDOWS_MAX; ++i) {
    if (manager->windows[i].id) { continue; }
    wnd.owner = app;
    if (!wnd.id) { wnd.id = rand(); }
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

void window_manager_bring_to_front(window_manager_t *manager, cig_id wnd_id) {
  register size_t i;
  for (i = 0; i < manager->count; ++i) {
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
  register size_t i;
  for (i = 0; i < manager->count; ++i) {
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
    
    const bool pressed = cig_pressed(CIG_INPUT_PRIMARY_ACTION, CIG_PRESS_INSIDE);
    
    cig_fill_panel(get_panel(PANEL_BUTTON), pressed ? CIG_PANEL_PRESSED : 0);
    
    if (cig_push_frame_insets(RECT_AUTO,  pressed ? cig_i_make(2, 3, 1, 2) : cig_i_make(1, 1, 2, 2))) {
      cig_draw_label((cig_text_properties) {
        .font = get_font(FONT_REGULAR),
        .max_lines = 1,
        .overflow = CIG_TEXT_SHOW_ELLIPSIS
      }, title);
      cig_pop_frame();
    }
    
    clicked = cig_clicked(CIG_INPUT_PRIMARY_ACTION, CIG_CLICK_DEFAULT_OPTIONS);
  }
  
  return clicked;
}

bool icon_button(cig_r rect, image_id_t image_id) {
  bool clicked = false;
  
  CIG(rect) {
    cig_enable_interaction();
    
    const bool pressed = cig_pressed(CIG_INPUT_PRIMARY_ACTION, CIG_PRESS_INSIDE);
    
    cig_fill_panel(get_panel(PANEL_BUTTON), pressed ? CIG_PANEL_PRESSED : 0);
    
    if (cig_push_frame_insets(RECT_AUTO, pressed ? cig_i_make(3, 3, 1, 1) : cig_i_make(2, 2, 2, 2))) {
      cig_draw_image(get_image(image_id), CIG_IMAGE_MODE_CENTER);
      cig_pop_frame();
    }
    
    clicked = cig_clicked(CIG_INPUT_PRIMARY_ACTION, CIG_CLICK_DEFAULT_OPTIONS);
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

    const bool pressed = cig_pressed(CIG_INPUT_PRIMARY_ACTION, CIG_PRESS_DEFAULT_OPTIONS);

    if (cig_clicked(CIG_INPUT_PRIMARY_ACTION, CIG_CLICK_DEFAULT_OPTIONS)) {
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
          cig_draw_image(get_image(IMAGE_CHECKMARK), CIG_IMAGE_MODE_CENTER);
        }
      }
    }
    CIG(_) {
      cig_draw_label((cig_text_properties) {
        .alignment.horizontal = CIG_TEXT_ALIGN_LEFT
      }, text);
    }
  }
  return toggled;
}

typedef struct {
  color_id_t text_color;
  bool has_focus;
  size_t count;
  bool selected[32];
  int *number_selected;
  struct {
    bool active;
    cig_v start;
    cig_r relative_rect;
  } drag_selection;
} file_browser_data_t;

bool begin_file_browser(cig_r rect, int direction, color_id_t text_color, bool parent_focused, int *number_selected) {
  if (!cig_push_grid(RECT_AUTO, cig_i_zero(), (cig_params) {
    .width = 75,
    .height = 75,
    .direction = direction
  })) {
    return false;
  }

  if (number_selected) {
    *number_selected = 0;
  }

  file_browser_data_t *data = CIG_ALLOCATE(file_browser_data_t);
  data->text_color = text_color;
  data->has_focus = parent_focused;
  data->number_selected = number_selected;
  data->count = 0;

  if (cig_visibility() == CIG_FRAME_APPEARED) {
    /* Clear file selection when file browser becomes visible */
    memset(data->selected, 0, sizeof(bool[32]));
  }

  cig_disable_culling();
  cig_enable_interaction();

  /*  Deselect everything */
  if (cig_pressed(CIG_INPUT_PRIMARY_ACTION, CIG_PRESS_DEFAULT_OPTIONS)) {
    memset(data->selected, 0, sizeof(bool[32]));

    if (!data->drag_selection.active && cig_input_state()->drag.active && cig_v_dist(cig_v_zero(), cig_input_state()->drag.change) > 2) {
      cig_input_state()->locked = true;
      data->drag_selection.active = true;
      data->drag_selection.start = cig_v_sub(cig_input_state()->drag.start_position, cig_v_make(CIG_SX, CIG_SY));
    }
  }

  if (cig_input_state()->drag.active) {
    if (data->drag_selection.active) {
      data->drag_selection.relative_rect = cig_r_clip(
        rect_of(
          data->drag_selection.start,
          cig_v_add(data->drag_selection.start, cig_input_state()->drag.change)
        ),
        cig_current()->rect
      );
    }
  } else {
    data->drag_selection.active = false;
  }

  return true;
}

bool file_item(image_id_t image, const char *title) {
  file_browser_data_t *data = CIG_READ(true, file_browser_data_t);
  cig_frame *file_frame;
  size_t index = data->count++;
  bool did_double_click = false, did_select = false;

  if (!(file_frame = large_file_icon(image,
                                     title,
                                     data->text_color,
                                     data->has_focus && data->selected[index],
                                     &did_double_click,
                                     data->has_focus ? &did_select : NULL))) {
    return false;
  }

  if (data->drag_selection.active) {
    data->selected[index] = cig_r_intersects(data->drag_selection.relative_rect, file_frame->rect);
  } else {
    if (did_select) { /* Single selection made */
      memset(data->selected, 0, sizeof(bool[32]));
      data->selected[index] = true;
    }
  }

  return did_double_click;
}

void end_file_browser() {
  register int i;
  file_browser_data_t *data = CIG_READ(true, file_browser_data_t);

  if (data->number_selected) {
    for (i = 0; i < 32; ++i) {
      if (data->selected[i]) {
        *(data->number_selected) += 1;
      }
    }
  }

  if (data->drag_selection.active) {
   cig_draw_rect(
      cig_r_offset(data->drag_selection.relative_rect, CIG_SX, CIG_SY),
      NULL,
      get_color(COLOR_BLACK),
      1
    );
  }

  cig_pop_frame();
}

void begin_menubar() {

}

void end_menubar() {

}

/*  ┌──────────┐
    │ INTERNAL │
    └──────────┘ */

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

  register size_t i;
  window_message_t msg;
  bool focused;

  for (i = 0; i < this->window_manager.count;) {
    window_t *wnd = this->window_manager.order[i];

    if (wnd->proc) {
      msg = 0;
      focused = false;
      if (!begin_window(wnd, &msg, &focused)) {
        continue;
      }
      wnd->proc(wnd, &msg, focused);
      switch (msg) {
        case WINDOW_CLOSE: {
          window_manager_close(&this->window_manager, wnd);
          end_window(wnd);
          continue;
        };
        default: break;
      }
      end_window(wnd);
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

static bool begin_window(window_t *wnd, window_message_t *msg, bool *focused) {
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
          CIG_CLAMP(window_drag.original_rect.x + cig_input_state()->drag.change.x, -(wnd->rect.w - 50), 640 - 30),
          CIG_CLAMP(window_drag.original_rect.y + cig_input_state()->drag.change.y, 0, 480 - 50),
          wnd->rect.w,
          wnd->rect.h
        );
      } else {
        window_drag.active = false;
      }
    }

    /*  This container is right-aligned, so X will be 0 */
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

static void end_window(window_t *wnd) {
  cig_pop_frame(); /* Content frame */

  /* Add resize thumb */
  cig_current()->insets = cig_i_zero();

  if (wnd->flags & IS_RESIZABLE) {
    CIG(RECT(CIG_W_INSET-16, CIG_H_INSET-16, 16, 16)) {
      cig_draw_image(get_image(IMAGE_RESIZE_HANDLE), CIG_IMAGE_MODE_TOP_LEFT);
    }
  }

  cig_pop_frame(); /* Window panel */
}

static cig_frame * large_file_icon(int icon,
                                   const char *title,
                                   color_id_t text_color,
                                   bool is_selected,
                                   bool *did_double_click,
                                   bool *did_select)
{
  cig_frame *file_element;
  
  CIG_CAPTURE(file_element, CIG_VSTACK(RECT_AUTO, CIG_INSETS(cig_i_make(2, 2, 2, 0)), CIG_PARAMS({
    CIG_SPACING(6)
  })) {
    cig_enable_interaction();

    if (did_select && cig_pressed(CIG_INPUT_PRIMARY_ACTION, CIG_PRESS_DEFAULT_OPTIONS)) {
      *did_select = true;
    }

    if (did_double_click) {
      *did_double_click = cig_clicked(CIG_INPUT_PRIMARY_ACTION, CIG_CLICK_DEFAULT_OPTIONS | CIG_CLICK_DOUBLE);
    }

    CIG(RECT_AUTO_H(32)) {
      if (is_selected) { enable_blue_selection_dithering(true); }
      cig_draw_image(get_image(icon), CIG_IMAGE_MODE_TOP);
      if (is_selected) { enable_blue_selection_dithering(false); }
    }

    cig_label *label = CIG_ALLOCATE(cig_label);
    label->spans = CIG_ALLOCATE(cig_span[16]);
    label->available_spans = 16;

    /* We need to prepare the label here to know how large of a rectangle
       to draw around it when the icon is selected */
    cig_label_prepare(label, CIG_SIZE_INSET, (cig_text_properties) {
        .color = is_selected ? get_color(COLOR_WHITE) : get_color(text_color),
        .alignment.vertical = CIG_TEXT_ALIGN_TOP
      }, title);

    CIG(_) {
      if (is_selected) {
        int text_x_in_parent = (CIG_W - label->bounds.w) * 0.5;
        cig_draw_rect(
          cig_r_make(CIG_SX+text_x_in_parent-1, CIG_SY-1, label->bounds.w+2, label->bounds.h+2),
          get_color(COLOR_WINDOW_ACTIVE_TITLEBAR),
          0,
          1
        );
      }
      cig_label_draw(label);
    }
  })

  return file_element;
}

static void open_explorer_at(const char *path) {
  application_t *explorer = win95_find_open_app("explorer");
  assert(explorer != NULL);
  window_manager_create(&this->window_manager, explorer, explorer_create_window(explorer, path));
}

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

static cig_r rect_of(cig_v p0, cig_v p1) {
  int32_t min_x = CIG_MIN(p0.x, p1.x);
  int32_t min_y = CIG_MIN(p0.y, p1.y);
  int32_t max_x = CIG_MAX(p0.x, p1.x);
  int32_t max_y = CIG_MAX(p0.y, p1.y);
  return cig_r_make(min_x, min_y, max_x-min_x, max_y-min_y);
}