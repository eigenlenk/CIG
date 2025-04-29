#include "welcome.h"
#include "cigcorem.h"

typedef struct {
  int tip_index;
} window_data_t;

static window_message_t process_main_window(window_t *this) {
  static const char *tips[] = {
    "This is not actually Windows 95 but a little demo application to develop and test a C immediate-mode GUI library called CIG.",
    "You can find the library on GitHub by visiting the World Wide Web link below:\n\nhttps://github.com/eigenlenk/cig\n\nMore text!",
    "A fatal exception 0E has occurred at 0F:DEADBEEF. The current application will be shot and terminated."
  };

	window_data_t *window_data = (window_data_t*)this->data;
	window_message_t msg = 0;

	begin_window(this);

	/* Main vertical stack */
	if (cig_push_vstack(RECT_AUTO, cig_insets_make(14, 16, 10, 16), (cig_layout_params_t) { .spacing = 12 })) {

		/* Row 1: Large title label */
		if (cig_push_frame(RECT_AUTO_H(20))) {
      cig_label(
        (cig_text_properties_t) {
          .font = get_font(FONT_TIMES_NEW_ROMAN_32_BOLD),
          .color = get_color(COLOR_BLACK),
          .alignment.horizontal = CIG_TEXT_ALIGN_LEFT,
          .flags = CIG_TEXT_FORMATTED
        },
        "Welcome to <font=%x>Windows</font><font=%x><color=%x>95</color></font>",
        get_font(FONT_ARIAL_BLACK_32),
        get_font(FONT_FRANKLIN_GOTHIC_BOOK_32),
        get_color(COLOR_WHITE)
      );
      cig_pop_frame();
    }

    /* Row 2: Horizontal stack consisting of tip view and stack of buttons next to it */
    if (cig_push_hstack(RECT_AUTO_H(154), cig_insets_uniform(1), (cig_layout_params_t) { .spacing = 12 })) {
    	if (cig_push_frame_insets(RECT_AUTO_W(330), cig_insets_make(14, 20, 14, 20))) {
        cig_fill_panel(get_panel(PANEL_LIGHT_YELLOW), 0);
        cig_fill_panel(get_panel(PANEL_INNER_BEVEL_NO_FILL), 0);
        if (cig_push_hstack(RECT_AUTO, cig_insets_zero(), (cig_layout_params_t) { .spacing = 14 })) {
        	if (cig_push_frame(cig_rect_make(0, 0, 32, 32))) {
        		cig_image(get_image(IMAGE_TIP_OF_THE_DAY), CIG_IMAGE_MODE_CENTER);
        		cig_pop_frame();
        	}
        	if (cig_push_vstack(RECT_AUTO, cig_insets_zero(), (cig_layout_params_t) { .spacing = 8 })) {
        		if (cig_push_frame(RECT_AUTO_H(32))) {
          		cig_label((cig_text_properties_t) {
          			.font = get_font(FONT_BOLD),
          			.alignment.horizontal = CIG_TEXT_ALIGN_LEFT
          		}, "Did you know...");
          		cig_pop_frame();
          	}
          	if (cig_push_frame(RECT_AUTO)) {
          		cig_label((cig_text_properties_t) {
          			.alignment.horizontal = CIG_TEXT_ALIGN_LEFT,
          			.alignment.vertical = CIG_TEXT_ALIGN_TOP,
          		}, tips[window_data->tip_index]);
          		cig_pop_frame();
          	}
          	cig_pop_frame();
        	}
        	cig_pop_frame();
        }
        cig_pop_frame();
      }

      if (cig_push_vstack(_, cig_insets_zero(), (cig_layout_params_t) { .spacing = 6, .height = 23 })) {
        standard_button(_, "What's New");
        standard_button(_, "Online Registration");
        if (standard_button(_, "Next Tip")) {
          window_data->tip_index = (window_data->tip_index+1)%3;
        }
        cig_spacer(58);
        if (cig_push_frame(RECT_AUTO_H(2))) { /* Separator */
        	cig_fill_panel(get_panel(PANEL_INNER_BEVEL_NO_FILL), 0);
        	cig_pop_frame();
        }
        cig_pop_frame();
      }
      cig_pop_frame();
    }

    /* Row 3: Checkbox and close button */
    if (cig_push_hstack(RECT_AUTO, cig_insets_zero(), (cig_layout_params_t) { .spacing = 12 })) {
			checkbox(RECT_AUTO_W(330), NULL, "Show this Welcome Screen next time you start Windows");
			standard_button(RECT_AUTO_H(23), "Close");
			cig_pop_frame();
		}

		cig_pop_frame();
  }

	end_window();

	return msg;
}

application_t welcome_app() {
  window_data_t *data = malloc(sizeof(window_data_t));
  data->tip_index = 0;

	return (application_t) {
		.windows = {
			(window_t) {
				.proc = &process_main_window,
				.data = data,
				.rect = RECT_CENTERED(488, 280),
				.title = "Welcome",
				.icon = -1
			}
		},
		.data = NULL
	};
}


        /*CIG_GRID({
          CIG_INSETS(cig_insets_uniform(10)),
          CIG_PARAMS({
            CIG_COLUMNS(3),
            CIG_ROWS(3),
            CIG_SPACING(5)
          })
        }) {
          CIG({ CIG_RECT(CIG_FILL) }) {
            cig_fill_color(get_color(COLOR_DESKTOP));
            cig_image(get_image(IMAGE_START_ICON), CIG_IMAGE_MODE_ASPECT_FIT);
          }

          CIG({ CIG_RECT(CIG_FILL) }) {
            cig_enable_interaction();
            bool *clipping_on = CIG_ALLOCATE(bool);
            if (cig_clicked(CIG_MOUSE_BUTTON_ANY, CIG_CLICK_DEFAULT_OPTIONS)) {
              *clipping_on = !*clipping_on;
            }
            cig_fill_color(get_color(COLOR_DESKTOP));
            if (*clipping_on) {
              cig_enable_clipping();
            }
            cig_image(get_image(IMAGE_START_ICON), CIG_IMAGE_MODE_ASPECT_FILL);
          }

          CIG({ CIG_RECT(CIG_FILL) }) {
            cig_fill_color(get_color(COLOR_DESKTOP));
            cig_image(get_image(IMAGE_START_ICON), CIG_IMAGE_MODE_SCALE_TO_FILL);
          }

          CIG({ CIG_RECT(CIG_FILL) }) {
            cig_fill_color(get_color(COLOR_DESKTOP));
            cig_image(get_image(IMAGE_START_ICON), CIG_IMAGE_MODE_CENTER);
            cig_image(get_image(IMAGE_START_ICON), CIG_IMAGE_MODE_TOP);
            cig_image(get_image(IMAGE_START_ICON), CIG_IMAGE_MODE_TOP_LEFT);
            cig_image(get_image(IMAGE_START_ICON), CIG_IMAGE_MODE_TOP_RIGHT);
            cig_image(get_image(IMAGE_START_ICON), CIG_IMAGE_MODE_LEFT);
            cig_image(get_image(IMAGE_START_ICON), CIG_IMAGE_MODE_RIGHT);
            cig_image(get_image(IMAGE_START_ICON), CIG_IMAGE_MODE_BOTTOM);
            cig_image(get_image(IMAGE_START_ICON), CIG_IMAGE_MODE_BOTTOM_LEFT);
            cig_image(get_image(IMAGE_START_ICON), CIG_IMAGE_MODE_BOTTOM_RIGHT);
          }
        }*/