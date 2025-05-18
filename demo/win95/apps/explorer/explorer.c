#include "explorer.h"
#include "cigcorem.h"

const char *explorer_path_my_computer = "mycomputer";
const char *explorer_path_recycle_bin = "recyclebin";

typedef struct {
  void (*content_builder)(bool);
} window_data_t;

static void window_proc(window_t *this, window_message_t *msg, bool window_focused) {
  window_data_t *window_data = (window_data_t*)this->data;
  cig_frame *primary_status = 0;

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
      CIG(_H(17)) {
        cig_enable_clipping();

        /* TODO: Status bar */
        CIG_CAPTURE(primary_status, CIG(_W(CIG_MIN(144, CIG_W)), cig_i_uniform(3)) {
          cig_fill_panel(get_panel(PANEL_INNER_BEVEL_NO_FILL), 0);
        })
        if (CIG_W > (144+2)) {
          int remaining_space = CIG_W - (144+2);
          CIG(RECT(CIG_W-remaining_space, 0, remaining_space, CIG_AUTO())) {
            cig_fill_panel(get_panel(PANEL_INNER_BEVEL_NO_FILL), 0);
          }
        }
      }

      CIG(_, CIG_INSETS(cig_i_uniform(2))) {
        cig_fill_solid(get_color(COLOR_WHITE));
        cig_fill_panel(get_panel(PANEL_FILES_CONTENT_BEVEL), 0);

        if (window_data->content_builder) {
          window_data->content_builder(window_focused);

          /*  Jumping allows us to go back into an already added frame. Here we have
              called the window content functor and know what status text to show.

              This particular situation has other solutions as well, main one being
              calculating the body size first, adding that frame and *then* inserting
              the status bar element. But using a jump means we can make the status bar
              visibility conditional and have the window content always fill whatever
              space is remaining without having to calculate it manually. */
          if (cig_jump(primary_status)) {
            cig_draw_label((cig_text_properties) {
              .alignment.horizontal = CIG_TEXT_ALIGN_LEFT
            }, "5 object(s)");
            cig_pop_frame();
          }
        }
      }
    }
  }
}

static void my_computer_content(bool window_focused) {
  if (!begin_file_browser(RECT_AUTO, CIG_LAYOUT_DIRECTION_HORIZONTAL, COLOR_BLACK, window_focused)) {
    return;
  }

  cig_enable_clipping();

  if (file_item(IMAGE_DRIVE_A_32, "3½ Floppy (A:)")) { }
  if (file_item(IMAGE_DRIVE_C_32, "(C:)")) { }
  if (file_item(IMAGE_DRIVE_D_32, "(D:)")) { }
  if (file_item(IMAGE_CONTROLS_FOLDER_32, "Control Panel")) { }

  end_file_browser();
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
    .min_size = { 220, 160 },
    .title = "My Computer",
    .icon = IMAGE_MY_COMPUTER_16,
    .flags = IS_RESIZABLE
  };
}
