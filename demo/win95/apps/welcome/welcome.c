#include "welcome.h"
#include "cigcorem.h"

typedef struct {

} window_data_t;

static window_message_t process_main_window(window_t *this) {
	window_data_t *window_data = (window_data_t*)this->data;
	window_message_t msg = 0;

	begin_window(this);

	CIG_VSTACK({
    CIG_RECT(cig_rect_make(0, 18, CIG_FILL_CONSTANT, CIG_H_INSET - 18)),
    CIG_INSETS(cig_insets_make(15, 17, 11, 18)),
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
        "Welcome to <font=%x>Windows</font><font=%x><color=%x>%d</color></font>",
        get_font(FONT_ARIAL_BLACK_32),
        get_font(FONT_FRANKLIN_GOTHIC_BOOK_32),
        get_color(COLOR_WHITE),
        95
      );
    }
    
    CIG(_) {
      CIG_HSTACK({
        CIG_INSETS(cig_insets_uniform(1)),
        CIG_PARAMS({
          CIG_SPACING(12)
        })
      }) {
        CIG({
          CIG_RECT(CIG_FILL_W(330))
        }) {
          cig_fill_panel(get_panel(PANEL_LIGHT_YELLOW), 0);
          cig_fill_panel(get_panel(PANEL_INNER_BEVEL_NO_FILL), 0);

          CIG_GRID({
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
          }
        }
        
        CIG_VSTACK({
          CIG_PARAMS({
            CIG_SPACING(6)
          })
        }) {
          standard_button(CIG_FILL_H(23), "What's New");
          standard_button(CIG_FILL_H(23), "Online Registration");
        }
      }
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