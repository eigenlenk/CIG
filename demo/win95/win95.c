#include "win95.h"
#include "raylib.h"

#define TASKBAR_H 28
#define RAYLIB_RECT(R) R.x, R.y, R.w, R.h

static bool draw_button_panel(bool pressed) {
  DrawRectangle(RAYLIB_RECT(cig_frame()->absolute_rect), (Color){ 195, 195, 195, 255 });
  
  if (!pressed) {
    DrawLine(CIG_GX, CIG_GY, CIG_GX + CIG_W - 1, CIG_GY, (Color){ 255, 255, 255, 255 });
    DrawLine(CIG_GX + 1, CIG_GY + 1, CIG_GX + 1, CIG_GY + CIG_H - 1, (Color){ 255, 255, 255, 255 });
    DrawLine(CIG_GX, CIG_GY + CIG_H - 1, CIG_GX + CIG_W, CIG_GY + CIG_H - 1, (Color){ 0, 0, 0, 255 });
    DrawLine(CIG_GX + CIG_W, CIG_GY, CIG_GX + CIG_W, CIG_GY + CIG_H - 1, (Color){ 0, 0, 0, 255 });
    DrawLine(CIG_GX + 1, CIG_GY + CIG_H - 2, CIG_GX + CIG_W - 1, CIG_GY + CIG_H - 2, (Color){ 130, 130, 130, 255 });
    DrawLine(CIG_GX + CIG_W - 1, CIG_GY + 1, CIG_GX + CIG_W - 1, CIG_GY + CIG_H - 2, (Color){ 130, 130, 130, 255 });
  } else {
    DrawLine(CIG_GX, CIG_GY, CIG_GX + CIG_W - 1, CIG_GY, (Color){ 0, 0, 0, 255 });
    DrawLine(CIG_GX + 1, CIG_GY + 1, CIG_GX + 1, CIG_GY + CIG_H - 1, (Color){ 0, 0, 0, 255 });
    DrawLine(CIG_GX, CIG_GY + CIG_H - 1, CIG_GX + CIG_W, CIG_GY + CIG_H - 1, (Color){ 255, 255, 255, 255 });
    DrawLine(CIG_GX + CIG_W, CIG_GY, CIG_GX + CIG_W, CIG_GY + CIG_H - 1, (Color){ 255, 255, 255, 255 }); 
    DrawLine(CIG_GX + 1, CIG_GY + CIG_H - 2, CIG_GX + CIG_W - 1, CIG_GY + CIG_H - 2, (Color){ 223, 223, 223, 255 });
    DrawLine(CIG_GX + CIG_W - 1, CIG_GY + 1, CIG_GX + CIG_W - 1, CIG_GY + CIG_H - 2, (Color){ 223, 223, 223, 255 });
    DrawLine(CIG_GX + 2, CIG_GY + 1, CIG_GX + CIG_W - 2, CIG_GY + 1, (Color){ 128, 128, 128, 255 });
    DrawLine(CIG_GX + 2, CIG_GY + 1, CIG_GX + 2, CIG_GY + CIG_H - 2, (Color){ 128, 128, 128, 255 });
  }
  
  return cig_push_frame_insets(
    CIG_FILL,
    pressed ? cig_insets_make(0, 3, 0, 2) : cig_insets_make(0, 1, 0, 2)
  );
}

static void draw_window_panel() {
  DrawRectangle(RAYLIB_RECT(cig_frame()->absolute_rect), (Color){ 195, 195, 195, 255 });
  
  DrawLine(CIG_GX, CIG_GY + CIG_H - 1, CIG_GX + CIG_W, CIG_GY + CIG_H - 1, (Color){ 0, 0, 0, 255 });
  DrawLine(CIG_GX + CIG_W, CIG_GY, CIG_GX + CIG_W, CIG_GY + CIG_H - 1, (Color){ 0, 0, 0, 255 });
  DrawLine(CIG_GX + 1, CIG_GY + CIG_H - 2, CIG_GX + CIG_W - 1, CIG_GY + CIG_H - 2, (Color){ 130, 130, 130, 255 });
  DrawLine(CIG_GX + CIG_W - 1, CIG_GY + 1, CIG_GX + CIG_W - 1, CIG_GY + CIG_H - 2, (Color){ 130, 130, 130, 255 });
  DrawLine(CIG_GX + 1, CIG_GY + 1, CIG_GX + CIG_W - 2, CIG_GY + 1, (Color){ 255, 255, 255, 255 });
  DrawLine(CIG_GX + 2, CIG_GY + 1, CIG_GX + 2, CIG_GY + CIG_H - 2, (Color){ 255, 255, 255, 255 });
  
  /*DrawLine(CIG_GX + 1, CIG_GY, CIG_GX + CIG_W - 1, CIG_GY, (Color){ 255, 255, 255, 255 });
  DrawLine(CIG_GX + 1, CIG_GY + 1, CIG_GX + 1, CIG_GY + CIG_H - 1, (Color){ 255, 255, 255, 255 });
  DrawLine(CIG_GX, CIG_GY + CIG_H - 1, CIG_GX + CIG_W, CIG_GY + CIG_H - 1, (Color){ 0, 0, 0, 255 });
  DrawLine(CIG_GX + CIG_W, CIG_GY, CIG_GX + CIG_W, CIG_GY + CIG_H - 1, (Color){ 0, 0, 0, 255 });
  DrawLine(CIG_GX + 1, CIG_GY + CIG_H - 2, CIG_GX + CIG_W - 1, CIG_GY + CIG_H - 2, (Color){ 130, 130, 130, 255 });
  DrawLine(CIG_GX + CIG_W - 1, CIG_GY + 1, CIG_GX + CIG_W - 1, CIG_GY + CIG_H - 2, (Color){ 130, 130, 130, 255 });*/
}

static void draw_inner_bevel_border() {
  DrawLine(CIG_GX, CIG_GY, CIG_GX + CIG_W - 1, CIG_GY, (Color) { 130, 130, 130, 255 });
  DrawLine(CIG_GX + 1, CIG_GY, CIG_GX + 1, CIG_GY + CIG_H - 1, (Color) { 130, 130, 130, 255 });
  DrawLine(CIG_GX, CIG_GY + CIG_H - 1, CIG_GX + CIG_W, CIG_GY + CIG_H - 1, (Color) { 255, 255, 255, 255 });
  DrawLine(CIG_GX + CIG_W, CIG_GY, CIG_GX + CIG_W, CIG_GY + CIG_H, (Color) { 255, 255, 255, 255 });
}

static void beveled_button(win95_t *this, cig_rect_t rect, const char *title) {
  CIG({
    CIG_RECT(rect)
  }) {
    cig_enable_interaction();
    if (draw_button_panel(cig_pressed(CIG_MOUSE_BUTTON_ANY, CIG_PRESS_INSIDE))) {
      cig_label((cig_text_properties_t) {
        .font = this->get_font(FONT_REGULAR)
      }, title);
      cig_pop_frame();
    }
  }
}

void run_win95(win95_t *this) {
  /* Desktop */
  if (cig_push_frame(cig_rect_make(0, 0, CIG_W, CIG_H - TASKBAR_H))) {
    DrawRectangle(RAYLIB_RECT(cig_frame()->absolute_rect), (Color){ 0, 130, 130, 255 });
    cig_pop_frame();
  }
  
  /* Taskbar */
  
  if (cig_push_frame_insets(cig_rect_make(0, CIG_H - TASKBAR_H, CIG_W, TASKBAR_H), cig_insets_make(2, 4, 2, 2))) {
    DrawRectangle(RAYLIB_RECT(cig_frame()->absolute_rect), (Color){ 195, 195, 195, 255 });
    DrawLine(CIG_GX, CIG_GY + 1, CIG_GX + CIG_W, CIG_GY + 1, (Color){ 255, 255, 255, 255 });
    
    CIG({
      CIG_RECT(CIG_FILL),
      CIG_PARAMS({
        CIG_AXIS(CIG_LAYOUT_AXIS_HORIZONTAL),
        CIG_SPACING(4)
      }),
      CIG_BUILDER(CIG_STACK_BUILDER)
    }) {
      CIG({
        CIG_RECT(CIG_FILL_W(54))
      }) {
        cig_enable_interaction();
        if (draw_button_panel(cig_pressed(CIG_MOUSE_BUTTON_ANY, CIG_PRESS_INSIDE))) {
          cig_label((cig_text_properties_t) { .font = this->get_font(FONT_BOLD) }, "Start");
          cig_pop_frame();
        }
      }
      
      CIG({
        CIG_RECT(CIG_FILL_W(95))
      }) {
        cig_enable_interaction();
        if (draw_button_panel(cig_pressed(CIG_MOUSE_BUTTON_ANY, CIG_PRESS_INSIDE))) {
          cig_label((cig_text_properties_t) { }, "Welcome");
          cig_pop_frame();
        }
      }
      
      CIG({
        CIG_RECT(CIG_FILL_W(95))
      }) {
        cig_enable_interaction();
        if (draw_button_panel(cig_pressed(CIG_MOUSE_BUTTON_ANY, CIG_PRESS_INSIDE))) {
          cig_label((cig_text_properties_t) { }, "My Computer");
          cig_pop_frame();
        }
      }
    }
    
    cig_pop_frame();
  }
  
  // "Welcome" window
  CIG({
    CIG_RECT(CIG_CENTERED(488, 280)),
    CIG_INSETS(cig_insets_uniform(3))
  }) {
    draw_window_panel();
    
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
      DrawRectangle(RAYLIB_RECT(cig_frame()->absolute_rect), (Color){ 0, 0, 130, 255 });
      
      cig_label((cig_text_properties_t) {
        .font = this->get_font(FONT_BOLD),
        .color = this->get_color(COLOR_TEXT_WHITE),
        .alignment.horizontal = CIG_TEXT_ALIGN_LEFT,
        .alignment.vertical = CIG_TEXT_ALIGN_MIDDLE
      }, "Welcome");
      
      CIG({
        CIG_RECT(cig_rect_make(CIG_W_INSET - 16, 0, 16, CIG_FILL_CONSTANT))
      }) {
        cig_enable_interaction();
        if (draw_button_panel(cig_pressed(CIG_MOUSE_BUTTON_ANY, CIG_PRESS_INSIDE))) {
          cig_label((cig_text_properties_t) { .font = this->get_font(FONT_BOLD) }, "X");
          cig_pop_frame();
        }
      }
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
            DrawTexturePro(*(Texture2D*)this->get_image(IMAGE_BRIGHT_YELLOW_PATTERN), (Rectangle) { 0, 0, CIG_W, CIG_H }, (Rectangle) { CIG_GX + 1, CIG_GY + 1, CIG_W - 2, CIG_H - 2 }, (Vector2){0,0}, 0, WHITE);
            draw_inner_bevel_border();
          }
          
          CIG({
            CIG_RECT(CIG_FILL),
            CIG_PARAMS({
              CIG_AXIS(CIG_LAYOUT_AXIS_VERTICAL),
              CIG_SPACING(6)
            }),
            CIG_BUILDER(CIG_STACK_BUILDER)
          }) {
            beveled_button(this, CIG_FILL_H(23), "What's New");
            beveled_button(this, CIG_FILL_H(23), "Online Registration");
          }
        }
      }
    }
  }
}