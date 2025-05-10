#include "explorer.h"
#include "cigcorem.h"

const char *explorer_path_my_computer = "mycomputer";
const char *explorer_path_recycle_bin = "recyclebin";

typedef struct {
  void (*content_builder)(void);
} window_data_t;

static window_message_t window_proc(window_t *this) {
  window_data_t *window_data = (window_data_t*)this->data;
  window_message_t msg = begin_window(this);

  CIG_VSTACK(_, CIG_PARAMS({
    CIG_SPACING(2)
  })) {
    CIG(_H(16)) {
      /* TODO: Menubar */
    }

    CIG_VSTACK(_, CIG_PARAMS({
      CIG_SPACING(2),
      CIG_ALIGNMENT_VERTICAL(CIG_LAYOUT_ALIGNS_BOTTOM)
    })) {
      CIG(_H(16)) {
        cig_enable_clipping();

        /* TODO: Status bar */
        CIG(_W(400)) {
          cig_fill_panel(get_panel(PANEL_INNER_BEVEL_NO_FILL), 0);
        }
      }

      CIG(_, CIG_INSETS(cig_i_uniform(2))) {
        cig_fill_solid(get_color(COLOR_WHITE));
        cig_fill_panel(get_panel(PANEL_FILES_CONTENT_BEVEL), 0);

        if (window_data->content_builder) {
          window_data->content_builder();
        }
      }
    }
  }

  end_window();
  
  return msg;
}

static void my_computer_content() {
  if (!cig_push_grid(RECT_AUTO, cig_i_zero(), (cig_layout_params_t) {
    .width = 75,
    .height = 75
  })) {
    return;
  }

  if (large_file_icon(IMAGE_DRIVE_A_32, "3½ Floppy (A:)", COLOR_BLACK)) { }
  if (large_file_icon(IMAGE_DRIVE_C_32, "(C:)", COLOR_BLACK)) { }
  if (large_file_icon(IMAGE_DRIVE_D_32, "(D:)", COLOR_BLACK)) { }

  cig_pop_frame();
}

application_t explorer_app() {
  return (application_t) {
    .id = "explorer"
  };
}

window_t explorer_create_window(application_t *app, const char *path) {
  window_data_t *data = malloc(sizeof(window_data_t));
  data->content_builder = &my_computer_content;

  return (window_t) {
    .id = cig_hash(path),
    .proc = &window_proc,
    .data = data,
    .rect = RECT_CENTERED(265, 220),
    .title = "My Computer",
    .icon = IMAGE_MY_COMPUTER_16
  };
}
