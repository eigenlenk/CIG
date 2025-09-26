#include "win95.h"
#include "apps/explorer/explorer.h"
#include "apps/welcome/welcome.h"
#include "apps/games/wordwiz/wordwiz.h"
#include "cigcorem.h"
#include <time.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

static win95_t *this = NULL;
static win95_menu start_menus[8];

enum {
  START_MAIN,
  START_PROGRAMS,
  START_PROGRAMS_ACCESSORIES,
  START_PROGRAMS_ACCESSORIES_GAMES,
  START_PROGRAMS_STARTUP,
  START_DOCUMENTS,
  START_SETTINGS,
  START_FIND
};

static void process_apps();
static void process_windows();
static void close_application(application_t *);
static void open_explorer_at(const char*);
static void launch_app_by_id(menu_item *);
static void setup_menus();

static void
about_wnd_proc(window_t *this, window_message_t *msg, bool window_focused);

static void start_button(cig_r rect) {
  CIG(rect, CIG_INSETS(cig_i_make(4, 2, 4, 2))) {
    cig_enable_interaction();
    cig_disable_culling();
    cig_enable_focus();

    menu_tracking_st *tracking = CIG_ALLOCATE(menu_tracking_st);

    menu_track(tracking, &start_menus[START_MAIN], (menu_presentation) {
      .position = { -4, -2 },
      .origin = ORIGIN_BOTTOM_LEFT,
    });

    cig_fill_style(get_style(STYLE_BUTTON), *tracking > 0 ? CIG_STYLE_APPLY_PRESS : 0);
    
    CIG_HSTACK(
      _,
      CIG_INSETS(*tracking > 0 ? cig_i_make(0, 1, 0, -1) : cig_i_zero()),
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
  };
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
    cig_fill_color(get_color(COLOR_DESKTOP_BG));
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
    cig_fill_color(get_color(COLOR_DIALOG_BACKGROUND));
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
      cig_fill_style(get_style(STYLE_INNER_BEVEL_NO_FILL), 0);
      cig_label_draw(&clock_st.label);
    }

    /* Center: Fill remaining middle space with task buttons */
    int taskbar_windows = 0;
    for (i = 0; i < WIN95_OPEN_WINDOWS_MAX; ++i) {
      window_t *wnd = &this->window_manager.windows[i];
      if (wnd->id && wnd->owner) {
        taskbar_windows++;
      }
    }

    CIG_HSTACK(
      cig_r_make(start_button_width+spacing, 0, CIG_W_INSET-start_button_width-clock_w-spacing*2, CIG_AUTO()),
      CIG_PARAMS({
        CIG_SPACING(spacing),
        CIG_COLUMNS(taskbar_windows),
        CIG_MAX_WIDTH(150)
      })
    ) {
      for (i = 0; i < WIN95_OPEN_WINDOWS_MAX; ++i) {
        window_t *wnd = &this->window_manager.windows[i];
        if (wnd->id && wnd->owner && taskbar_button(RECT_AUTO, wnd->title, wnd->icon, wnd->id == cig_focused_id())) {
          window_manager_bring_to_front(&this->window_manager, wnd->id);
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

  setup_menus();
}

void run_win95(win95_t *win95) {
  this = win95;

  process_apps();
  do_desktop();
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

void
win95_show_about_window()
{
  window_manager_create(&this->window_manager, NULL, (window_t) {
    .id = cig_hash("aboutwin"),
    .proc = &about_wnd_proc,
    .rect = CENTER_APP_WINDOW(360, 280),
    .title = "About Windooze",
    .icon = -1,
    .flags = IS_UNIQUE_WINDOW
  });
}

/*  ┌────────────────┐
    │ WINDOW MANAGER │
    └────────────────┘ */

window_t* window_manager_create(window_manager_t *manager, application_t *app, window_t wnd) {
  register size_t i;
  if (!manager) { manager = &this->window_manager; }
  if (!wnd.id) { wnd.id = rand(); }

  if (wnd.flags & IS_UNIQUE_WINDOW) {
    window_t *existing_wnd = window_manager_find_id(manager, wnd.id);

    if (existing_wnd) {
      window_manager_bring_to_front(manager, existing_wnd->id);
      return existing_wnd;
    }
  }

  for (i = 0; i < WIN95_OPEN_WINDOWS_MAX; ++i) {
    if (manager->windows[i].id) { continue; }
    wnd.owner = app;

    manager->windows[i] = wnd;
    manager->order[manager->count++] = &manager->windows[i];
    cig_set_focused_id(wnd.id);
    return manager->order[manager->count-1];
  }

  return NULL;
}

void window_manager_close(window_manager_t *manager, window_t *wnd) {
  register size_t i, j;
  if (!manager) { manager = &this->window_manager; }
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

void window_manager_maximize(window_manager_t *manager, window_t *wnd) {
  if (wnd->flags & IS_MAXIMIZED) {
    wnd->flags &= ~IS_MAXIMIZED;
    wnd->rect = wnd->rect_before_maximized;
  } else {
    wnd->flags |= IS_MAXIMIZED;
    wnd->rect_before_maximized = wnd->rect;
    wnd->rect = cig_r_make(0, 0, cig_layout_rect().w, cig_layout_rect().h - TASKBAR_H);
  }
}

void window_manager_minimize(window_manager_t *manager, window_t *wnd) {
  if (wnd->flags & IS_MINIMIZED) {
    return;
  }
  /* TODO: Actually Win95 focuses on whatever was focused *before* selecting
   * this window. So either the desktop or some some window.
   */
  cig_set_focused_id(0);
  wnd->flags |= IS_MINIMIZED;
}

void window_manager_bring_to_front(window_manager_t *manager, cig_id wnd_id) {
  register size_t i;
  if (!manager) { manager = &this->window_manager; }
  for (i = 0; i < manager->count; ++i) {
    if (manager->order[i]->id == wnd_id) {
      window_t *wnd = manager->order[i];
      wnd->flags &= ~IS_MINIMIZED;
      manager->order[i] = manager->order[manager->count-1];
      manager->order[manager->count-1] = wnd;
      cig_set_focused_id(wnd_id);
      break;
    }
  }
}

window_t* window_manager_find_primary_window(window_manager_t *manager, application_t *app) {
  register size_t i;
  if (!manager) { manager = &this->window_manager; }
  for (i = 0; i < manager->count; ++i) {
    if (manager->order[i]->owner == app && manager->order[i]->flags & IS_PRIMARY_WINDOW) {
      return manager->order[i];
    }
  }
  return NULL;
}

window_t* window_manager_find_id(window_manager_t *manager, cig_id id) {
  register size_t i;
  if (!manager) { manager = &this->window_manager; }
  for (i = 0; i < manager->count; ++i) {
    if (manager->order[i]->id == id) {
      return manager->order[i];
    }
  }
  return NULL;
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

    if (!(wnd->flags & IS_MINIMIZED)) {
      msg = 0;
      focused = false;
      if (!window_begin(wnd, &msg, &focused)) {
        continue;
      }
      if (wnd->proc) {
        wnd->proc(wnd, &msg, focused);
      }
      switch (msg) {
      case WINDOW_CLOSE:
        {
          window_end(wnd);
          window_manager_close(&this->window_manager, wnd);
        } continue;
      case WINDOW_MAXIMIZE:
        {
          window_manager_maximize(&this->window_manager, wnd);
        } break;
      case WINDOW_MINIMIZE:
        {
          window_manager_minimize(&this->window_manager, wnd);
        } break;
      default:
        break;
      }
      window_end(wnd);
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

static void open_explorer_at(const char *path) {
  application_t *explorer = win95_find_open_app("explorer");
  assert(explorer != NULL);
  window_t *wnd;
  if ((wnd = window_manager_find_id(&this->window_manager, cig_hash(path)))) {
    window_manager_bring_to_front(&this->window_manager, wnd->id);
  } else {
    window_manager_create(&this->window_manager, explorer, explorer_create_window(explorer, path));
  }
}

static void launch_app_by_id(menu_item *item) {
  const char *app_id = (const char *)item->data;
  printf("Launch application: %s\n", app_id);
  
  if (!strcmp(app_id, "wordwiz")) {
    win95_open_app(wordwiz_app());
  }
}

/* Set up the main START menu and its children */
static void
setup_menus()
{
  /* The one and only. Best to ever do it. */
  menu_setup(&start_menus[START_MAIN], "Start", START, NULL, 2, (menu_group[]) {
    {
      .items = {
        .count = 6,
        .list = {
          { .type = CHILD_MENU, .data = &start_menus[START_PROGRAMS], .icon = IMAGE_PROGRAM_FOLDER_24 },
          { .type = CHILD_MENU, .data = &start_menus[START_DOCUMENTS], .icon = IMAGE_DOCUMENTS_24 },
          { .type = CHILD_MENU, .data = &start_menus[START_SETTINGS], .icon = IMAGE_SETTINGS_24 },
          { .type = CHILD_MENU, .data = &start_menus[START_FIND], .icon = IMAGE_FIND_24 },
          { .title = "Help", .icon = IMAGE_HELP_24 },
          { .title = "Run...", .icon = IMAGE_RUN_24 },
        }
      }
    },
    {
      .items = {
        .count = 1,
        .list = {
          { .title = "Shut Down...", .icon = IMAGE_SHUT_DOWN_24 },
        }
      }
    },
  });

  /* Start -> Programs */
  menu_setup(&start_menus[START_PROGRAMS], "Programs", START_SUBMENU, NULL, 1, (menu_group[]) {
    {
      .items = {
        .count = 6,
        .list = {
          { .type = CHILD_MENU, .data = &start_menus[START_PROGRAMS_ACCESSORIES], .icon = IMAGE_PROGRAM_FOLDER_16 },
          { .type = CHILD_MENU, .data = &start_menus[START_PROGRAMS_STARTUP], .icon = IMAGE_PROGRAM_FOLDER_16 },
          { .title = "Microsoft Exchange", .icon = IMAGE_MAIL_16 },
          { .title = "MS-DOS Prompt", .icon = IMAGE_MSDOS_16 },
          { .title = "The Microsoft Network", .icon = IMAGE_MSN_16 },
          { .title = "Windows Explorer", .icon = IMAGE_EXPLORER_16 }
        }
      }
    }
  });

  /* Start -> Documents */
  menu_setup(&start_menus[START_DOCUMENTS], "Documents", START_SUBMENU, NULL, 1, (menu_group[]) {
    {
      .items = {
        .count = 1,
        .list = {
          { .title = "(Empty)", .type = DISABLED }
        }
      }
    }
  });

  /* Start -> Settings */
  menu_setup(&start_menus[START_SETTINGS], "Settings", START_SUBMENU, NULL, 1, (menu_group[]) {
    {
      .items = {
        .count = 1,
        .list = {
          { .title = "(Empty)", .type = DISABLED }
        }
      }
    }
  });

  /* Start -> Find */
  menu_setup(&start_menus[START_FIND], "Find", START_SUBMENU, NULL, 1, (menu_group[]) {
    {
      .items = {
        .count = 1,
        .list = {
          { .title = "(Empty)", .type = DISABLED }
        }
      }
    }
  });

  /* Start -> Programs -> Games */
  menu_setup(&start_menus[START_PROGRAMS_ACCESSORIES_GAMES], "Games", START_SUBMENU, NULL, 1, (menu_group[]) {
    {
      .items = {
        .count = 1,
        .list = {
          { .title = "WordWiz", .icon = IMAGE_WORDWIZ_16, .data = "wordwiz", .handler = &launch_app_by_id }
        }
      }
    }
  });

  /* Start -> Programs -> Accessories */
  menu_setup(&start_menus[START_PROGRAMS_ACCESSORIES], "Accessories", START_SUBMENU, NULL, 1, (menu_group[]) {
    {
      .items = {
        .count = 3,
        .list = {
          { .type = CHILD_MENU, .data = &start_menus[START_PROGRAMS_ACCESSORIES_GAMES], .icon = IMAGE_PROGRAM_FOLDER_16 },
          { .title = "Calculator", .icon = IMAGE_CALCULATOR_16 },
          { .title = "Notepad", .icon = IMAGE_NOTEPAD_16 },
          { .title = "Paint", .icon = IMAGE_PAINT_16 }
        }
      }
    }
  });

  /* Start -> Programs -> StartUp */
  menu_setup(&start_menus[START_PROGRAMS_STARTUP], "StartUp", START_SUBMENU, NULL, 1, (menu_group[]) {
    {
      .items = {
        .count = 1,
        .list = {
          { .title = "(Empty)", .type = DISABLED }
        }
      }
    }
  });
}

static unsigned long long
total_system_memory()
{
  static unsigned long long available_memory = 0;
  if (!available_memory) {
#ifdef _WIN32
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);
    available_memory = status.ullTotalPhys;
#else
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    available_memory = pages * page_size;
#endif
  }
  return available_memory;
}

/* About Windooze 95 window */
static void
about_wnd_proc(window_t *this, window_message_t *msg, bool window_focused)
{
  CIG(_, CIG_INSETS(cig_i_uniform(12))) {
    CIG_HSTACK(_, CIG_PARAMS({
      CIG_SPACING(3)
    })) {
      CIG(RECT_SIZED(64, 64)) {
        cig_draw_image(get_image(IMAGE_LOGO_TEXT), CIG_IMAGE_MODE_CENTER);
      }
      CIG(_, CIG_INSETS(cig_i_make(0, 9, 0, 0))) {
        CIG_VSTACK(
          _,
          CIG_PARAMS({
            CIG_SPACING(5),
            CIG_ALIGNMENT_VERTICAL(CIG_LAYOUT_ALIGNS_BOTTOM)
          })
        ) {
          CIG(_H(23), CIG_PARAMS({
            CIG_ALIGNMENT_HORIZONTAL(CIG_LAYOUT_ALIGNS_RIGHT)
          })) {
            if (standard_button(_W(74), "OK")) {
              *msg = WINDOW_CLOSE;
            }
          }

          CIG_VSTACK(_, CIG_PARAMS({
            CIG_SPACING(5),
          })) {
            CIG(_H(11)) {
              cig_draw_label((cig_text_properties) { .alignment.horizontal = CIG_TEXT_ALIGN_LEFT }, "Macrohard Windooze");
            }
            CIG(_H(11)) {
              cig_draw_label((cig_text_properties) { .alignment.horizontal = CIG_TEXT_ALIGN_LEFT }, "Windooze 95");
            }
            CIG(_H(11)) {
              cig_draw_label((cig_text_properties) { .alignment.horizontal = CIG_TEXT_ALIGN_LEFT }, "Copyright © 2025 github.com/eigenlenk");
            }

            cig_spacer(28);

            CIG(_H(11)) {
              cig_draw_label((cig_text_properties) { .alignment.horizontal = CIG_TEXT_ALIGN_LEFT }, "This product is licensed to:");
            }
            CIG(_H(11)) {
              cig_draw_label((cig_text_properties) { .alignment.horizontal = CIG_TEXT_ALIGN_LEFT }, "Bill Gates");
            }
            CIG(_H(11)) {
              cig_draw_label((cig_text_properties) { .alignment.horizontal = CIG_TEXT_ALIGN_LEFT }, "Blue Screen Technologies");
            }

            cig_spacer(4);
            CIG(_H(2)) { /* Separator */
              cig_fill_style(get_style(STYLE_INNER_BEVEL_NO_FILL), 0);
            }
            cig_spacer(2);

            CIG(_H(11)) {
              cig_draw_label((cig_text_properties) {
                .alignment.horizontal = CIG_TEXT_ALIGN_LEFT
              }, "Physical Memory Available to Windooze:");

              cig_draw_label(
                (cig_text_properties) {
                  .alignment.horizontal = CIG_TEXT_ALIGN_RIGHT,
                  .flags = CIG_TEXT_FORMATTED
                },
                "%lld MB",
                total_system_memory() / (1024 * 1024)
              );
            }

            CIG(_H(11)) {
              time_t t = time(NULL);
              struct tm *ct = localtime(&t);

              cig_draw_label((cig_text_properties) {
                .alignment.horizontal = CIG_TEXT_ALIGN_LEFT
              }, "System Resources:");

              cig_draw_label(
                (cig_text_properties) {
                  .alignment.horizontal = CIG_TEXT_ALIGN_RIGHT,
                  .flags = CIG_TEXT_FORMATTED
                },
                "%d%% Free",
                100 - ct->tm_sec
              );
            }
          }
        }
      }
    }
  }
}