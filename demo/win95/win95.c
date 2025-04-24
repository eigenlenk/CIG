#include "win95.h"

#define TASKBAR_H 28

static bool standard_button(win95_t *this, cig_rect_t rect, const char *title) {
  bool clicked = false;
  
  CIG({
    CIG_RECT(rect)
  }) {
    cig_enable_interaction();
    
    const bool pressed = cig_pressed(CIG_MOUSE_BUTTON_ANY, CIG_PRESS_INSIDE);
    
    cig_fill_panel(this->get_panel(PANEL_BUTTON), pressed ? CIG_PANEL_PRESSED : 0);
    
    if (cig_push_frame_insets(CIG_FILL,  pressed ? cig_insets_make(1, 3, 0, 2) : cig_insets_make(0, 1, 0, 2))) {
      cig_label((cig_text_properties_t) {
        .font = this->get_font(FONT_REGULAR)
      }, title);
      cig_pop_frame();
    }
    
    clicked = cig_clicked(CIG_MOUSE_BUTTON_ANY, CIG_CLICK_DEFAULT_OPTIONS);
  }
  
  return clicked;
}

static bool taskbar_button(win95_t *this, cig_rect_t rect, const char *title) {
  bool clicked = false;
  
  CIG({
    CIG_RECT(rect)
  }) {
    cig_enable_interaction();
    
    const bool pressed = cig_pressed(CIG_MOUSE_BUTTON_ANY, CIG_PRESS_INSIDE);
    
    cig_fill_panel(this->get_panel(PANEL_BUTTON), pressed ? CIG_PANEL_PRESSED : 0);
    
    if (cig_push_frame_insets(CIG_FILL,  pressed ? cig_insets_make(0, 3, 0, 2) : cig_insets_make(0, 1, 0, 2))) {
      cig_label((cig_text_properties_t) {
        .font = this->get_font(FONT_REGULAR)
      }, title);
      cig_pop_frame();
    }
    
    clicked = cig_clicked(CIG_MOUSE_BUTTON_ANY, CIG_CLICK_DEFAULT_OPTIONS);
  }
  
  return clicked;
}


void run_win95(win95_t *this) {
  /* Desktop */
  if (cig_push_frame(cig_rect_make(0, 0, CIG_W, CIG_H - TASKBAR_H))) {
    cig_fill_color(this->get_color(COLOR_DESKTOP));
    cig_pop_frame();
  }
  
  /* Taskbar */
  
  if (cig_push_frame_insets(cig_rect_make(0, CIG_H - TASKBAR_H, CIG_W, TASKBAR_H), cig_insets_make(2, 4, 2, 2))) {
    cig_fill_color(this->get_color(COLOR_DIALOG_BACKGROUND));
    cig_draw_line(this->get_color(COLOR_TEXT_WHITE), cig_vec2_make(0, 1), cig_vec2_make(CIG_W, 1), 1);
    
    CIG({
      CIG_RECT(CIG_FILL),
      CIG_PARAMS({
        CIG_AXIS(CIG_LAYOUT_AXIS_HORIZONTAL),
        CIG_SPACING(4)
      }),
      CIG_BUILDER(CIG_STACK_BUILDER)
    }) {
      taskbar_button(this, CIG_FILL_W(54), "Start");
      taskbar_button(this, CIG_FILL_W(95), "Welcome");
      taskbar_button(this, CIG_FILL_W(95), "My Computer");
    }
    
    cig_pop_frame();
  }
  
  // "Welcome" window
  CIG({
    CIG_RECT(CIG_CENTERED(488, 280)),
    CIG_INSETS(cig_insets_uniform(3))
  }) {
    cig_fill_panel(this->get_panel(PANEL_STANDARD_DIALOG), 0);

    /* Titlebar */
    CIG({
      CIG_RECT(CIG_FILL_H(18)),
      CIG_INSETS(cig_insets_uniform(2)),
      CIG_PARAMS({
        CIG_AXIS(CIG_LAYOUT_AXIS_HORIZONTAL),
        CIG_SPACING(2),
        CIG_ALIGNMENT_HORIZONTAL(CIG_LAYOUT_ALIGNS_RIGHT)
      }),
      CIG_BUILDER(CIG_STACK_BUILDER)
    }) {
      cig_fill_color(this->get_color(COLOR_WINDOW_ACTIVE_TITLEBAR));

      cig_label((cig_text_properties_t) {
        .font = this->get_font(FONT_BOLD),
        .color = this->get_color(COLOR_TEXT_WHITE),
        .alignment.horizontal = CIG_TEXT_ALIGN_LEFT,
        .alignment.vertical = CIG_TEXT_ALIGN_MIDDLE
      }, "Welcome");
      
      standard_button(this, cig_rect_make(CIG_W_INSET - 16, 0, 16, CIG_FILL_CONSTANT), "X");
    }
    
    /* Body */
    CIG({
      CIG_RECT(cig_rect_make(0, 18, CIG_FILL_CONSTANT, CIG_H_INSET - 18)),
      CIG_INSETS(cig_insets_make(15, 17, 11, 18)),
      CIG_PARAMS({
        CIG_AXIS(CIG_LAYOUT_AXIS_VERTICAL),
        CIG_SPACING(12)
      }),
      CIG_BUILDER(CIG_STACK_BUILDER)
    }) {
      CIG({
        CIG_RECT(CIG_FILL_H(20))
      }) {
        cig_label(
          (cig_text_properties_t) {
            .font = this->get_font(FONT_TIMES_NEW_ROMAN_32_BOLD),
            .color = this->get_color(COLOR_TEXT_BLACK),
            .alignment.horizontal = CIG_TEXT_ALIGN_LEFT,
            .flags = CIG_TEXT_FORMATTED
          },
          "Welcome to <font=%x>Windows</font><font=%x><color=%x>%d</color></font>",
          this->get_font(FONT_ARIAL_BLACK_32),
          this->get_font(FONT_FRANKLIN_GOTHIC_BOOK_32),
          this->get_color(COLOR_TEXT_WHITE),
          95
        );
      }
      
      CIG(_) {
        CIG({
          CIG_RECT(CIG_FILL),
          CIG_INSETS(cig_insets_uniform(1)),
          CIG_PARAMS({
            CIG_AXIS(CIG_LAYOUT_AXIS_HORIZONTAL),
            CIG_SPACING(12)
          }),
          CIG_BUILDER(CIG_STACK_BUILDER)
        }) {
          CIG({
            CIG_RECT(CIG_FILL_W(330))
          }) {
            cig_fill_panel(this->get_panel(PANEL_LIGHT_YELLOW), 0);
            cig_fill_panel(this->get_panel(PANEL_INNER_BEVEL_NO_FILL), 0);
          }
          
          CIG({
            CIG_RECT(CIG_FILL),
            CIG_PARAMS({
              CIG_AXIS(CIG_LAYOUT_AXIS_VERTICAL),
              CIG_SPACING(6)
            }),
            CIG_BUILDER(CIG_STACK_BUILDER)
          }) {
            standard_button(this, CIG_FILL_H(23), "What's New");
            standard_button(this, CIG_FILL_H(23), "Online Registration");
          }
        }
      }
    }
  }
}