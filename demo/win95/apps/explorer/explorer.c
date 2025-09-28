#include "explorer.h"
#include "components/window.h"
#include "components/menu.h"
#include "components/file_browser.h"

#include <string.h>
#include <stdio.h>

#define STATUS_TEXT_LEN 64

const char *explorer_path_my_computer = "mycomputer";
const char *explorer_path_recycle_bin = "recyclebin";

/*
 * These are example menu handlers which can be assigned on 3 levels:
 * 1. One handler per whole menu
 * 2. One handler per group
 * 3. Handler for each menu item
 */
static void default_menu_handler(win95_menu *, menu_group *, menu_item *);
static void edit_menu_clipboard_operations(menu_group *, menu_item *);
static void menu_close_action(menu_item *);

enum {
  MENU_FILE,
  MENU_FILE_NEW,
  MENU_EDIT,
  MENU_VIEW,
  MENU_VIEW_ARRANGE_ICONS,
  MENU_HELP  
};

typedef struct {
  void (*menubar_builder)(window_t *, bool);
  void (*content_builder)(window_t *, bool, char *);
  bool status_bar_visible;
  int number_of_files_selected;
  win95_menu menus[16];
} window_data_t;

static void window_proc(window_t *this, bool window_focused) {
  window_data_t *window_data = (window_data_t*)this->data;
  const bool menubar_visible = window_data->menubar_builder != NULL;
  char status_text[STATUS_TEXT_LEN] = "";

  cig_frame *content = cig_current();
  cig_frame *file_content;

  cig_set_next_id(CIG_TINYHASH(this->id, cig_hash("content")));
  CIG_RETAIN(file_content, CIG(
    CIG_RECT(BUILD_RECT(
      PIN(LEFT_OF(content)),
      PIN(RIGHT_OF(content)),
      PIN(TOP_OF(content), OFFSET_BY(18)),
      PIN(BOTTOM_OF(content), OFFSET_BY(window_data->status_bar_visible ? -20 : 0))
    )),
    CIG_INSETS(cig_i_uniform(2))
  ) {
    cig_fill_color(get_color(COLOR_WHITE));
    cig_fill_style(get_style(STYLE_FILES_CONTENT_BEVEL), 0);

    if (window_data->content_builder) {
      window_data->content_builder(this, window_focused, &status_text[0]);
    }
  })

  if (window_data->status_bar_visible) {
    CIG(BUILD_RECT(
      PIN(LEFT_OF(content)),
      PIN(RIGHT_OF(content)),
      PIN(BELOW(file_content), OFFSET_BY(2)),
      PIN(BOTTOM_OF(content))
    )) {
      cig_enable_clipping();

      CIG(_W(CIG_MIN(144, CIG_W)), cig_i_make(3, 2, 3, 2)) {
        cig_fill_style(get_style(STYLE_INNER_BEVEL_NO_FILL), 0);
        cig_draw_label((cig_text_properties) {
          .alignment.horizontal = CIG_TEXT_ALIGN_LEFT
        }, status_text);
      }

      if (CIG_W > (144+2)) {
        int remaining_space = CIG_W - (144+2);
        CIG(RECT(CIG_W-remaining_space, 0, remaining_space, CIG_AUTO())) {
          cig_fill_style(get_style(STYLE_INNER_BEVEL_NO_FILL), 0);
        }

        if (CIG_W > (144+2)) {
          int remaining_space = CIG_W - (144+2);
          CIG(RECT(CIG_W-remaining_space, 0, remaining_space, CIG_AUTO())) {
            cig_fill_style(get_style(STYLE_INNER_BEVEL_NO_FILL), 0);
          }
        }
      }

      if (!(this->flags & IS_MAXIMIZED)) {
        CIG(RECT(CIG_W_INSET-12, CIG_H_INSET-12, 12, 12)) {
          cig_draw_image(get_image(IMAGE_RESIZE_HANDLE), CIG_IMAGE_MODE_TOP_LEFT);
        }
      }
    }
  }

  if (menubar_visible) {
    CIG(BUILD_RECT(
      PIN(LEFT_OF(content)),
      PIN(RIGHT_OF(content)),
      PIN(ABOVE(file_content), OFFSET_BY(-2)),
      PIN(TOP_OF(content))
    )) {
      window_data->menubar_builder(this, window_focused);
    }
  }
}

static void my_computer_content(window_t *wnd, bool window_focused, char *status_text) {
  window_data_t *window_data = (window_data_t*)wnd->data;

  if (!begin_file_browser(RECT_AUTO, CIG_LAYOUT_DIRECTION_HORIZONTAL, COLOR_BLACK, window_focused, &window_data->number_of_files_selected)) {
    return;
  }

  if (file_item(IMAGE_DRIVE_A_32, "3½ Floppy (A:)")) { }
  if (file_item(IMAGE_DRIVE_C_32, "(C:)")) { }
  if (file_item(IMAGE_DRIVE_D_32, "(D:)")) { }
  if (file_item(IMAGE_CONTROLS_FOLDER_32, "Control Panel")) { }

  end_file_browser();

  if (window_data->number_of_files_selected > 0) {
    snprintf(status_text, STATUS_TEXT_LEN, "%d object(s) selected", window_data->number_of_files_selected);
  } else {
    strcpy(status_text, "4 object(s)");
  }
}

static void recycle_bin_content(window_t *wnd, bool window_focused, char *status_text) {
  strcpy(status_text, "");
}

static void standard_menubar_builder(window_t *wnd, bool window_focused) {
  window_data_t *window_data = (window_data_t*)wnd->data;

  menu_setup(&window_data->menus[MENU_FILE_NEW], "New", DROPDOWN, &default_menu_handler, 2, (menu_group[]) {
    {
      .items = {
        .count = 2,
        .list = {
          { .title = "Folder" },
          { .title = "Shortcut" },
        }
      }
    },
    {
      .items = {
        .count = 1,
        .list = {
          { .title = "Text Document" },
        }
      }
    },
  });

  menu_setup(&window_data->menus[MENU_FILE], "File", DROPDOWN, &default_menu_handler, 3, (menu_group[]) {
    {
      .items = {
        .count = 1,
        .list = {
          { .type = CHILD_MENU, .data = &window_data->menus[MENU_FILE_NEW] }
        }
      }
    },
    {
      .items = {
        .count = 4,
        .list = {
          { .title = "Create Shortcut", .type = DISABLED },
          { .title = "Delete", .type = DISABLED },
          { .title = "Rename", .type = DISABLED },
          { .title = "Properties", .type = DISABLED }
        }
      }
    },
    {
      .items = {
        .count = 1,
        .list = {
          { .title = "Close", .handler = &menu_close_action, .data = wnd }
        }
      }
    },
  });

  const bool clipboard_operations_disabled = window_data->number_of_files_selected == 0;

  menu_setup(&window_data->menus[MENU_EDIT], "Edit", DROPDOWN, &default_menu_handler, 3, (menu_group[]) {
    {
      .items = {
        .count = 1,
        .list = {
          { .title = "Undo" }
        }
      }
    },
    {
      .items = {
        .count = 4,
        .list = {
          { .title = "Cut", .type = clipboard_operations_disabled ? DISABLED : NONE },
          { .title = "Copy", .type = clipboard_operations_disabled ? DISABLED : NONE },
          { .title = "Paste", .type = clipboard_operations_disabled ? DISABLED : NONE },
          { .title = "Paste Shortcut", .type = clipboard_operations_disabled ? DISABLED : NONE },
        }
      },
      .handler = &edit_menu_clipboard_operations
    },
    {
      .items = {
        .count = 2,
        .list = {
          { .title = "Select All" },
          { .title = "Invert Selection" },
        }
      }
    }
  });

  menu_setup(&window_data->menus[MENU_VIEW_ARRANGE_ICONS], "Arrange Icons", DROPDOWN, &default_menu_handler, 2, (menu_group[]) {
    {
      .items = {
        .count = 4,
        .list = {
          { .title = "by Name" },
          { .title = "by Type" },
          { .title = "by Size" },
          { .title = "by Date" },
        }
      }
    },
    {
      .items = {
        .count = 1,
        .list = {
          { .title = "Auto Arrange" },
        }
      }
    },
  });

  menu_setup(&window_data->menus[MENU_VIEW], "View", DROPDOWN, &default_menu_handler, 4, (menu_group[]) {
    {
      .items = {
        .count = 2,
        .list = {
          { .title = "Toolbar" },
          { .title = "Status Bar", .type = TOGGLE, .data = &window_data->status_bar_visible },
        }
      }
    },
    {
      .items = {
        .count = 4,
        .list = {
          { .title = "Large Icons", .type = RADIO_ON },
          { .title = "Small Icons" },
          { .title = "List" },
          { .title = "Details" },
        }
      }
    },
    {
      .items = {
        .count = 2,
        .list = {
          { .type = CHILD_MENU, .data = &window_data->menus[MENU_VIEW_ARRANGE_ICONS] },
          { .title = "Line up Icons" },
        }
      }
    },
    {
      .items = {
        .count = 2,
        .list = {
          { .title = "Refresh" },
          { .title = "Options" },
        }
      }
    }
  });

  menu_setup(&window_data->menus[MENU_HELP], "Help", DROPDOWN, &default_menu_handler, 2, (menu_group[]) {
    {
      .items = {
        .count = 1,
        .list = {
          { .title = "Help Topics" }
        }
      }
    },
    {
      .items = {
        .count = 1,
        .list = {
          { .title = "About Windooze 95", ._handler = &win95_show_about_window }
        }
      }
    },
  });

  menubar(4, (win95_menu*[]) {
    &window_data->menus[MENU_FILE],
    &window_data->menus[MENU_EDIT],
    &window_data->menus[MENU_VIEW],
    &window_data->menus[MENU_HELP],
  });
}

static void default_menu_handler(win95_menu *menu, menu_group *group, menu_item *item) {
  printf("Clicked: %s -> %s (Group ID: %llx)\n", menu->title, item->title, (unsigned long long)group->id);
}

static void edit_menu_clipboard_operations(menu_group *group, menu_item *item) {
  printf("Handle clipboard operation: %s\n", item->title);
}

static void menu_close_action(menu_item *close_item) {
  window_send_message((window_t *)close_item->data, WINDOW_CLOSE);
}

/* */

application_t explorer_app() {
  return (application_t) {
    .id = "explorer"
  };
}

window_t explorer_create_window(application_t *app, const char *path) {
  window_data_t *data = malloc(sizeof(window_data_t));
  data->status_bar_visible = true;
  data->number_of_files_selected = 0;

  if (path == explorer_path_my_computer) {
    data->menubar_builder = &standard_menubar_builder;
    data->content_builder = &my_computer_content;
    return (window_t) {
      .id = cig_hash(path),
      .proc = &window_proc,
      .data = data,
      .rect = CENTER_APP_WINDOW(265, 220),
      .min_size = { 220, 160 },
      .title = "My Computer",
      .icon = IMAGE_MY_COMPUTER_16,
      .flags = IS_RESIZABLE
    };
  } else if (path == explorer_path_recycle_bin) {
    data->menubar_builder = NULL;
    data->content_builder = &recycle_bin_content;
    return (window_t) {
      .id = cig_hash(path),
      .proc = &window_proc,
      .data = data,
      .rect = CENTER_APP_WINDOW(265, 220),
      .min_size = { 220, 160 },
      .title = "Recycle Bin",
      .icon = IMAGE_BIN_EMPTY_16,
      .flags = IS_RESIZABLE
    };
  } else {
    assert(false);
  }
}
