#include "win95.h"
#include "time.h"
#include "cigcorem.h"

#define TASKBAR_H 28

static bool standard_button(win95_t *this, cig_rect_t rect, const char *title) {
  bool clicked = false;
  
  CIG({
    CIG_RECT(rect)
  }) {
    cig_enable_interaction();
    
    const bool pressed = cig_pressed(CIG_MOUSE_BUTTON_ANY, CIG_PRESS_INSIDE);
    
    cig_fill_panel(this->get_panel(PANEL_BUTTON), pressed ? CIG_PANEL_PRESSED : 0);
    
    if (cig_push_frame_insets(CIG_FILL,  pressed ? cig_insets_make(1, 3, -1, 2) : cig_insets_make(0, 1, 0, 2))) {
      cig_label((cig_text_properties_t) {
        .font = this->get_font(FONT_REGULAR)
      }, title);
      cig_pop_frame();
    }
    
    clicked = cig_clicked(CIG_MOUSE_BUTTON_ANY, CIG_CLICK_DEFAULT_OPTIONS);
  }
  
  return clicked;
}

static bool taskbar_button(
  win95_t *this,
  cig_rect_t rect,
  const char *title,
  int icon,
  bool selected
) {
  bool clicked = false;
  
  CIG({
    CIG_RECT(rect),
    CIG_INSETS(cig_insets_make(4, 2, 4, 2))
  }) {
    cig_enable_interaction();
    
    const bool pressed = cig_pressed(CIG_MOUSE_BUTTON_ANY, CIG_PRESS_INSIDE);
    
    cig_fill_panel(this->get_panel(PANEL_BUTTON), selected ? CIG_PANEL_SELECTED : (pressed ? CIG_PANEL_PRESSED : 0));
    
    CIG_HSTACK({
      CIG_INSETS(selected ? cig_insets_make(0, 1, 0, -1) : cig_insets_zero()),
      CIG_PARAMS({
        CIG_SPACING(2)
      })
    }) {
      if (icon >= 0) {
        CIG({
          CIG_RECT(CIG_FILL_W(16))
        }) {
          cig_image(this->get_image(icon), CIG_IMAGE_MODE_CENTER);
        }
      }
      if (title) {
        CIG(_) {
          cig_label((cig_text_properties_t) {
            .font = this->get_font(selected ? FONT_BOLD : FONT_REGULAR),
            .alignment.horizontal = CIG_TEXT_ALIGN_LEFT,
            .overflow = CIG_TEXT_SHOW_ELLIPSIS
          }, title);
        }
      }
    }

    clicked = cig_clicked(CIG_MOUSE_BUTTON_ANY, CIG_CLICK_DEFAULT_OPTIONS);
  }
  
  return clicked;
}

static bool start_button(win95_t *this, cig_rect_t rect) {
  bool clicked = false;
  
  CIG({
    CIG_RECT(rect),
    CIG_INSETS(cig_insets_make(4, 2, 4, 2))
  }) {
    cig_enable_interaction();
    
    const bool pressed = cig_pressed(CIG_MOUSE_BUTTON_ANY, CIG_PRESS_INSIDE);
    
    cig_fill_panel(this->get_panel(PANEL_BUTTON), pressed ? CIG_PANEL_PRESSED : 0);
    
    CIG_HSTACK({
      CIG_INSETS(pressed ? cig_insets_make(0, 1, 0, -1) : cig_insets_zero()),
      CIG_PARAMS({
        CIG_SPACING(2)
      })
    }) {
      CIG({ CIG_RECT(CIG_FILL_W(16)) }) {
        cig_image(this->get_image(IMAGE_START_ICON), CIG_IMAGE_MODE_CENTER);
      }

      CIG(_) {
        cig_label((cig_text_properties_t) {
          .font = this->get_font(FONT_BOLD),
          .alignment.horizontal = CIG_TEXT_ALIGN_LEFT
        }, "Start");
      }
    }

    clicked = cig_clicked(CIG_MOUSE_BUTTON_ANY, CIG_CLICK_DEFAULT_OPTIONS);
  }
  
  return clicked;
}

static void do_desktop(win95_t *this) {
  if (cig_push_frame(cig_rect_make(0, 0, CIG_W, CIG_H - TASKBAR_H))) {
    cig_fill_color(this->get_color(COLOR_DESKTOP));
    cig_pop_frame();
  }
}

static void do_taskbar(win95_t *this) {
  static label_t some_label, clock_label;
  static unsigned long cnt = 0;

  const int start_button_width = 54;
  const int spacing = 4;

  CIG({
    CIG_RECT(cig_rect_make(0, CIG_H - TASKBAR_H, CIG_W, TASKBAR_H)),
    CIG_INSETS(cig_insets_make(2, 4, 2, 2))
  }) {
    cig_fill_color(this->get_color(COLOR_DIALOG_BACKGROUND));
    cig_draw_line(this->get_color(COLOR_WHITE), cig_vec2_make(0, 1), cig_vec2_make(CIG_W, 1), 1);

    /* Left: Start button */
    start_button(this, CIG_FILL_W(start_button_width));
    
    /* Right: Calculate clock bounds */
    time_t t = time(NULL);
    struct tm *ct = localtime(&t);

    cig_prepare_label(&clock_label, (cig_text_properties_t) { .flags = CIG_TEXT_FORMATTED }, 0, "%02d:%02d", ct->tm_hour, ct->tm_min);

    const int clock_w = (clock_label.bounds.w+11*2);

    CIG({
      CIG_RECT(cig_rect_make(CIG_W_INSET-clock_w, 0, clock_w, CIG_FILL_CONSTANT)),
      CIG_INSETS(cig_insets_uniform(1))
    }) {
      cig_fill_panel(this->get_panel(PANEL_INNER_BEVEL_NO_FILL), 0);
      cig_prepared_label(&clock_label);
    }

    /* Center: Fill remaining middle space with task buttons */

    CIG_HSTACK({
      CIG_RECT(cig_rect_make(start_button_width+spacing, 0, CIG_W_INSET-start_button_width-clock_w-spacing*2, CIG_FILL_CONSTANT)),
      CIG_PARAMS({ CIG_SPACING(spacing) })
    }) {
      taskbar_button(this, CIG_FILL_W(95), "Welcome", -1, false);
      taskbar_button(this, CIG_FILL_W(95), "My Computer", IMAGE_MY_COMPUTER_16, true);

      cig_prepare_label(&some_label, (cig_text_properties_t) { .flags = CIG_TEXT_FORMATTED }, 0, "Counter: %lu", cnt=cnt+(1+cnt*0.001));

      CIG({
        CIG_RECT(CIG_FILL_W(some_label.bounds.w))
      }) {
        cig_prepared_label(&some_label);
      }

      taskbar_button(this, CIG_FILL_W(95), "Control Panel", -1, false);
    }
  }
}

void run_win95(win95_t *this) {
  do_desktop(this);
  do_taskbar(this);
  
  // "Welcome" window
  CIG({
    CIG_RECT(CIG_CENTERED(488, 280)),
    CIG_INSETS(cig_insets_uniform(3))
  }) {
    cig_fill_panel(this->get_panel(PANEL_STANDARD_DIALOG), 0);

    /* Titlebar */
    CIG_HSTACK({
      CIG_RECT(CIG_FILL_H(18)),
      CIG_INSETS(cig_insets_uniform(2)),
      CIG_PARAMS({
        CIG_SPACING(2),
        CIG_ALIGNMENT_HORIZONTAL(CIG_LAYOUT_ALIGNS_RIGHT)
      })
    }) {
      cig_fill_color(this->get_color(COLOR_WINDOW_ACTIVE_TITLEBAR));

      cig_label((cig_text_properties_t) {
        .font = this->get_font(FONT_BOLD),
        .color = this->get_color(COLOR_WHITE),
        .alignment.horizontal = CIG_TEXT_ALIGN_LEFT,
        .alignment.vertical = CIG_TEXT_ALIGN_MIDDLE
      }, "Welcome");
      
      standard_button(this, cig_rect_make(CIG_W_INSET - 16, 0, 16, CIG_FILL_CONSTANT), "X");
    }
    
    /* Body */
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
            .font = this->get_font(FONT_TIMES_NEW_ROMAN_32_BOLD),
            .color = this->get_color(COLOR_BLACK),
            .alignment.horizontal = CIG_TEXT_ALIGN_LEFT,
            .flags = CIG_TEXT_FORMATTED
          },
          "Welcome to <font=%x>Windows</font><font=%x><color=%x>%d</color></font>",
          this->get_font(FONT_ARIAL_BLACK_32),
          this->get_font(FONT_FRANKLIN_GOTHIC_BOOK_32),
          this->get_color(COLOR_WHITE),
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
            cig_fill_panel(this->get_panel(PANEL_LIGHT_YELLOW), 0);
            cig_fill_panel(this->get_panel(PANEL_INNER_BEVEL_NO_FILL), 0);

            CIG_GRID({
              CIG_INSETS(cig_insets_uniform(10)),
              CIG_PARAMS({
                CIG_COLUMNS(3),
                CIG_ROWS(3),
                CIG_SPACING(5)
              })
            }) {
              CIG({ CIG_RECT(CIG_FILL) }) {
                cig_fill_color(this->get_color(COLOR_DESKTOP));
                cig_image(this->get_image(IMAGE_START_ICON), CIG_IMAGE_MODE_ASPECT_FIT);
              }

              CIG({ CIG_RECT(CIG_FILL) }) {
                cig_enable_interaction();
                bool *clipping_on = CIG_ALLOCATE(bool);
                if (cig_clicked(CIG_MOUSE_BUTTON_ANY, CIG_CLICK_DEFAULT_OPTIONS)) {
                  *clipping_on = !*clipping_on;
                }
                cig_fill_color(this->get_color(COLOR_DESKTOP));
                if (*clipping_on) {
                  cig_enable_clipping();
                }
                cig_image(this->get_image(IMAGE_START_ICON), CIG_IMAGE_MODE_ASPECT_FILL);
              }

              CIG({ CIG_RECT(CIG_FILL) }) {
                cig_fill_color(this->get_color(COLOR_DESKTOP));
                cig_image(this->get_image(IMAGE_START_ICON), CIG_IMAGE_MODE_SCALE_TO_FILL);
              }

              CIG({ CIG_RECT(CIG_FILL) }) {
                cig_fill_color(this->get_color(COLOR_DESKTOP));
                cig_image(this->get_image(IMAGE_START_ICON), CIG_IMAGE_MODE_CENTER);
                cig_image(this->get_image(IMAGE_START_ICON), CIG_IMAGE_MODE_TOP);
                cig_image(this->get_image(IMAGE_START_ICON), CIG_IMAGE_MODE_TOP_LEFT);
                cig_image(this->get_image(IMAGE_START_ICON), CIG_IMAGE_MODE_TOP_RIGHT);
                cig_image(this->get_image(IMAGE_START_ICON), CIG_IMAGE_MODE_LEFT);
                cig_image(this->get_image(IMAGE_START_ICON), CIG_IMAGE_MODE_RIGHT);
                cig_image(this->get_image(IMAGE_START_ICON), CIG_IMAGE_MODE_BOTTOM);
                cig_image(this->get_image(IMAGE_START_ICON), CIG_IMAGE_MODE_BOTTOM_LEFT);
                cig_image(this->get_image(IMAGE_START_ICON), CIG_IMAGE_MODE_BOTTOM_RIGHT);
              }
            }
          }
          
          CIG_VSTACK({
            CIG_PARAMS({
              CIG_SPACING(6)
            })
          }) {
            standard_button(this, CIG_FILL_H(23), "What's New");
            standard_button(this, CIG_FILL_H(23), "Online Registration");
          }
        }
      }
    }
  }
}