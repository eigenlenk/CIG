#include "welcome.h"

typedef struct {
  int tip_index;
} window_data_t;

static void process_main_window(window_t *this, window_message_t *msg, bool window_focused) {
  static const char *tips[] = {
    "This is not actually Windows 95 but a little demo application to develop and test a C immediate-mode GUI library called CIG.",
    "You can find the library on GitHub by visiting the World Wide Web link below:\n\nhttps://github.com/eigenlenk/cig",
    "A fatal exception 0E has occurred at 0F:DEADBEEF. The current application will be shot and terminated."
  };

  window_data_t *window_data = (window_data_t*)this->data;

  /*  Using grid to lay out this window. By default, grid items flow to right
      so we can just append elements and they will jump to the next row when
      the column in filled. */
  CIG_GRID(RECT_AUTO, CIG_INSETS(cig_i_make(14, 16, 10, 16)), CIG_PARAMS({
    CIG_SPACING(12)
  })) {
    /* Title text */
    CIG(RECT_AUTO_H(20)) {
      cig_draw_label(
        (cig_text_properties) {
          .font = get_font(FONT_TIMES_NEW_ROMAN_32_BOLD),
          .color = get_color(COLOR_BLACK),
          .alignment.horizontal = CIG_TEXT_ALIGN_LEFT,
          .max_lines = 1,
          .flags = CIG_TEXT_FORMATTED
        },
        "Welcome to <font=%x>Windooze</font><font=%x><color=%x>95</color></font>",
        get_font(FONT_ARIAL_BLACK_32),
        get_font(FONT_FRANKLIN_GOTHIC_BOOK_32),
        get_color(COLOR_WHITE)
      );
    }

    /* Tip text and container */
    CIG(RECT_SIZED(330, 154), CIG_INSETS(cig_i_make(14, 20, 14, 20))) {
      cig_fill_style(get_style(STYLE_LIGHT_YELLOW), 0);
      cig_fill_style(get_style(STYLE_INNER_BEVEL_NO_FILL), 0);

      CIG_HSTACK(_, NO_INSETS, CIG_PARAMS({
        CIG_SPACING(14)
      })) {
        CIG(RECT_SIZED(32, 32)) {
          cig_draw_image(get_image(IMAGE_TIP_OF_THE_DAY), CIG_IMAGE_MODE_CENTER);
        }
        CIG_VSTACK(_, NO_INSETS, CIG_PARAMS({
          CIG_SPACING(8)
        })) {
          CIG(RECT_AUTO_H(32)) {
            cig_draw_label((cig_text_properties) {
              .font = get_font(FONT_BOLD),
              .alignment.horizontal = CIG_TEXT_ALIGN_LEFT
            }, "Did you know...");
          }
          CIG(_) {
            cig_draw_label((cig_text_properties) {
              .alignment.horizontal = CIG_TEXT_ALIGN_LEFT,
              .alignment.vertical = CIG_TEXT_ALIGN_TOP
            }, tips[window_data->tip_index]);
          }
        }
      }
    }

    /* List of buttons on the right */
    CIG_VSTACK(_, CIG_PARAMS({
      CIG_SPACING(6),
      CIG_HEIGHT(23)
    })) {
      standard_button(_, "What's New");
      standard_button(_, "Online Registration");
      if (standard_button(_, "Next Tip")) {
        window_data->tip_index = (window_data->tip_index+1)%3;
      }
      CIG(RECT_FILL, NO_INSETS, CIG_PARAMS({
        CIG_ALIGNMENT_VERTICAL(CIG_LAYOUT_ALIGNS_BOTTOM)
      })) {
        CIG(_H(2)) { /* Separator */
          cig_fill_style(get_style(STYLE_INNER_BEVEL_NO_FILL), 0);
        }
      }
    }

    CIG(RECT_AUTO_W(330)) {
      checkbox(_, NULL, "Show this Welcome Screen next time you start Windows");
    }

    CIG(_) {
      if (standard_button(RECT_CENTERED_VERTICALLY(RECT_AUTO_H(23)), "Close")) {
        *msg = WINDOW_CLOSE;
      }
    }
  }
}

application_t welcome_app() {
  window_data_t *data = malloc(sizeof(window_data_t));
  data->tip_index = 0;

  return (application_t) {
    .id = "welcome",
    .windows = {
      (window_t) {
        .id = cig_hash("welcome"),
        .proc = &process_main_window,
        .data = data,
        .rect = CENTER_APP_WINDOW(488, 280),
        .title = "Welcome",
        .icon = -1,
        .flags = IS_PRIMARY_WINDOW
      }
    },
    .data = NULL,
    .flags = KILL_WHEN_PRIMARY_WINDOW_CLOSED
  };
}