#include "welcome.h"
#include "cigcorem.h"

typedef struct {

} window_data_t;

static window_message_t process_main_window(window_t *this) {
	window_data_t *window_data = (window_data_t*)this->data;
	window_message_t msg = 0;

	begin_window(this);

	CIG_VSTACK({
    CIG_INSETS(cig_insets_make(14, 17, 10, 18)),
    CIG_PARAMS({
      CIG_SPACING(12)
    })
  }) {
    CIG({
    	CIG_RECT(CIG_FILL_H(20))
   	}) {
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
    }
    
    CIG_HSTACK({
    	CIG_RECT(CIG_FILL_H(154)),
      CIG_INSETS(cig_insets_uniform(1)),
      CIG_PARAMS({
        CIG_SPACING(12)
      })
    }) {
      CIG({
        CIG_RECT(CIG_FILL_W(330)),
        CIG_INSETS(cig_insets_make(14, 20, 14, 20))
      }) {
        cig_fill_panel(get_panel(PANEL_LIGHT_YELLOW), 0);
        cig_fill_panel(get_panel(PANEL_INNER_BEVEL_NO_FILL), 0);

        CIG_HSTACK({
        	CIG_PARAMS({
        		CIG_SPACING(14)
        	})
        }) {
        	CIG({
        		CIG_RECT(CIG_SIZED(32, 32))
        	}) {
        		cig_image(get_image(IMAGE_TIP_OF_THE_DAY), CIG_IMAGE_MODE_CENTER);
        	}

        	CIG_VSTACK({
        		CIG_PARAMS({ CIG_SPACING(8) })
        	}) {
        		CIG({ CIG_RECT(CIG_FILL_H(32)) }) {
          		cig_label((cig_text_properties_t) {
          			.font = get_font(FONT_BOLD),
          			.alignment.horizontal = CIG_TEXT_ALIGN_LEFT
          		}, "Did you know...");
          	}

          	CIG(_) {
          		cig_label((cig_text_properties_t) {
          			.alignment.horizontal = CIG_TEXT_ALIGN_LEFT,
          			.alignment.vertical = CIG_TEXT_ALIGN_TOP,
          		}, "This is not actually Windows 95 but a little demo application to develop and test a C immediate-mode GUI library called CIG.");
          	}
        	}
        }
      }
      
      CIG_VSTACK({
        CIG_PARAMS({
          CIG_SPACING(6)
        })
      }) {
        standard_button(CIG_FILL_H(23), "What's New");
        standard_button(CIG_FILL_H(23), "Online Registration");
        standard_button(CIG_FILL_H(23), "Next Tip");
        cig_spacer(56);
        CIG({ CIG_RECT(CIG_FILL_H(2)) }) { cig_fill_panel(get_panel(PANEL_INNER_BEVEL_NO_FILL), 0); }
      }
    }

		CIG_HSTACK({
			CIG_PARAMS({
				CIG_SPACING(12)
			})
		}) {
			checkbox(CIG_FILL_W(330), NULL, "Show this Welcome Screen next time you start Windows");
			standard_button(CIG_FILL_H(23), "Close");
		}
  }

	end_window();

	return msg;
}

application_t welcome_app() {
	return (application_t) {
		.windows = {
			(window_t) {
				.proc = &process_main_window,
				.data = malloc(sizeof(window_data_t)),
				.rect = CIG_CENTERED(488, 280),
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