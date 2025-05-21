#include "explorer.h"
#include "cigcorem.h"

#include <string.h>
#include <stdio.h>

#define STATUS_TEXT_LEN 64

const char *explorer_path_my_computer = "mycomputer";
const char *explorer_path_recycle_bin = "recyclebin";

typedef struct {
  void (*menubar_builder)(window_t *, bool);
  void (*content_builder)(bool, char *);
  bool status_bar_visible;
} window_data_t;

static void window_proc(window_t *this, window_message_t *msg, bool window_focused) {
  window_data_t *window_data = (window_data_t*)this->data;
  const bool menubar_visible = window_data->menubar_builder != NULL;
  char status_text[STATUS_TEXT_LEN] = "";

  CIG_VSTACK(_, CIG_PARAMS({
    CIG_SPACING(2)
  })) {
    if (menubar_visible) {
      CIG(_H(16)) {
        window_data->menubar_builder(this, window_focused);
      }
    }

    CIG(_H(CIG_H - (menubar_visible?(16+2):0) - (window_data->status_bar_visible?(18+2):0)), CIG_INSETS(cig_i_uniform(2))) {
      cig_fill_solid(get_color(COLOR_WHITE));
      cig_fill_panel(get_panel(PANEL_FILES_CONTENT_BEVEL), 0);

      if (window_data->content_builder) {
        window_data->content_builder(window_focused, &status_text[0]);
      }
    }

    if (window_data->status_bar_visible) {
      CIG(_H(18)) {
        cig_enable_clipping();

        CIG(_W(CIG_MIN(144, CIG_W)), cig_i_make(3, 2, 3, 2)) {
          cig_fill_panel(get_panel(PANEL_INNER_BEVEL_NO_FILL), 0);
          cig_draw_label((cig_text_properties) {
            .alignment.horizontal = CIG_TEXT_ALIGN_LEFT
          }, status_text);
        }

        if (CIG_W > (144+2)) {
          int remaining_space = CIG_W - (144+2);
          CIG(RECT(CIG_W-remaining_space, 0, remaining_space, CIG_AUTO())) {
            cig_fill_panel(get_panel(PANEL_INNER_BEVEL_NO_FILL), 0);
          }
        }

        CIG(RECT(CIG_W_INSET-12, CIG_H_INSET-12, 12, 12)) {
          cig_draw_image(get_image(IMAGE_RESIZE_HANDLE), CIG_IMAGE_MODE_TOP_LEFT);
        }
      }
    }
  }
}

static void my_computer_content(bool window_focused, char *status_text) {
  int number_selected;

  if (!begin_file_browser(RECT_AUTO, CIG_LAYOUT_DIRECTION_HORIZONTAL, COLOR_BLACK, window_focused, &number_selected)) {
    return;
  }

  cig_enable_clipping();

  if (file_item(IMAGE_DRIVE_A_32, "3½ Floppy (A:)")) { }
  if (file_item(IMAGE_DRIVE_C_32, "(C:)")) { }
  if (file_item(IMAGE_DRIVE_D_32, "(D:)")) { }
  if (file_item(IMAGE_CONTROLS_FOLDER_32, "Control Panel")) { }

  end_file_browser();

  if (number_selected > 0) {
    snprintf(status_text, STATUS_TEXT_LEN, "%d object(s) selected", number_selected);
  } else {
    strcpy(status_text, "4 object(s)");
  }
}

static void recycle_bin_content(bool window_focused, char *status_text) {
  strcpy(status_text, "");
}

static void standard_menubar_builder(window_t *wnd, bool window_focused) {
  begin_menubar();

  end_menubar();
}

application_t explorer_app() {
  return (application_t) {
    .id = "explorer"
  };
}

window_t explorer_create_window(application_t *app, const char *path) {
  window_data_t *data = malloc(sizeof(window_data_t));
  data->status_bar_visible = true;

  if (path == explorer_path_my_computer) {
    data->menubar_builder = &standard_menubar_builder;
    data->content_builder = &my_computer_content;
    return (window_t) {
      .id = cig_hash(path),
      .proc = &window_proc,
      .data = data,
      .rect = RECT_CENTERED(265, 220),
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
      .rect = RECT_CENTERED(265, 220),
      .min_size = { 220, 160 },
      .title = "Recycle Bin",
      .icon = IMAGE_BIN_EMPTY_16,
      .flags = IS_RESIZABLE
    };
  } else {
    assert(false);
  }
}
