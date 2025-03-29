#include "raylib.h"
#include "cig.h"
#include <stdio.h>
#include <string.h>

#define RECT(R) R.x, R.y, R.w, R.h

static cig_context_t ctx = { 0 };

enum Fonts {
  FONT_REGULAR = 0,
  FONT_BOLD,
  FONT_TIMES_NEW_ROMAN_32_BOLD,
  
  __FONT_COUNT
};

enum Colors {
  COLOR_BLACK = 0,
  COLOR_WHITE
};

static struct font_store {
  Font font;
} fonts[__FONT_COUNT];

static Color colors[16] = { 0 };

static struct {
  Texture2D yellow_white_pattern;
} res;

static void demo_ui();
static void render_text(cig_rect_t, const char*, size_t, cig_font_ref, cig_text_color_ref);
static cig_vec2_t measure_text(const char*, size_t, cig_font_ref);
static cig_font_info_t font_query(cig_font_ref);

int main(int argc, const char *argv[]) {
  SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_ALWAYS_RUN);
  InitWindow(640, 480, "Windooze 95");
  SetTargetFPS(60);
  // ToggleFullscreen();

  res.yellow_white_pattern = LoadTexture("patyello.png");

  fonts[FONT_REGULAR].font = LoadFont("winr.fnt");
  fonts[FONT_BOLD].font = LoadFont("winb.fnt");
  fonts[FONT_TIMES_NEW_ROMAN_32_BOLD].font = LoadFont("tnr32b.fnt");
  
  SetTextureFilter(fonts[FONT_REGULAR].font.texture, TEXTURE_FILTER_POINT);
  SetTextureFilter(fonts[FONT_BOLD].font.texture, TEXTURE_FILTER_POINT);
  SetTextureFilter(fonts[FONT_TIMES_NEW_ROMAN_32_BOLD].font.texture, TEXTURE_FILTER_POINT);
  
  colors[COLOR_BLACK] = (Color) { 0, 0, 0, 255 };
  colors[COLOR_WHITE] = (Color) { 255, 255, 255, 255 };
  
  cig_init_context(&ctx);
  cig_set_text_render_callback(&render_text);
  cig_set_text_measure_callback(&measure_text);
  cig_set_font_query_callback(&font_query);
  cig_set_default_font(&fonts[FONT_REGULAR].font);

  while (!WindowShouldClose()) {
    BeginDrawing();
    // ClearBackground(RAYWHITE);
    
    
    cig_begin_layout(&ctx, NULL, cig_rect_make(0, 0, GetScreenWidth(), GetScreenHeight()));
    cig_set_input_state(
      cig_vec2_make(GetMouseX(), GetMouseY()),
      IsMouseButtonDown(MOUSE_BUTTON_LEFT) ? CIG_INPUT_MOUSE_BUTTON_LEFT : 0 +
      IsMouseButtonDown(MOUSE_BUTTON_RIGHT) ? CIG_INPUT_MOUSE_BUTTON_RIGHT : 0
    );
    demo_ui();
    cig_end_layout();
    
    // DrawText("Congrats! You created your first window!", 190, 200, 20, LIGHTGRAY);
    
    EndDrawing();
  }

  CloseWindow();

  return 0;
}

#define TASKBAR_H 28

static bool draw_button_panel(bool pressed) {
  DrawRectangle(RECT(cig_frame()->absolute_rect), (Color){ 195, 195, 195, 255 });
  
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
    pressed ? cig_insets_make(0, 2, 0, 0) : cig_insets_zero()
  );
}

static void draw_window_panel() {
  DrawRectangle(RECT(cig_frame()->absolute_rect), (Color){ 195, 195, 195, 255 });
  
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

static void beveled_button(cig_rect_t rect, const char *title) {
  CIG({
    CIG_RECT(rect)
  }) {
    cig_enable_interaction();
    if (draw_button_panel(cig_pressed(CIG_MOUSE_BUTTON_ANY, CIG_PRESS_INSIDE))) {
      cig_label(title, (cig_text_properties_t) {
        .font = &fonts[FONT_REGULAR],
        .alignment.horizontal = CIG_TEXT_ALIGN_CENTER,
        .alignment.vertical = CIG_TEXT_ALIGN_MIDDLE
      });
      cig_pop_frame();
    }
  }
}

static void demo_ui() {
  /* Desktop */
  if (cig_push_frame(cig_rect_make(0, 0, CIG_W, CIG_H - TASKBAR_H))) {
    DrawRectangle(RECT(cig_frame()->absolute_rect), (Color){ 0, 130, 130, 255 });
    cig_pop_frame();
  }
  
  /* Taskbar */
  
  if (cig_push_frame_insets(cig_rect_make(0, CIG_H - TASKBAR_H, CIG_W, TASKBAR_H), cig_insets_make(2, 4, 2, 2))) {
    DrawRectangle(RECT(cig_frame()->absolute_rect), (Color){ 195, 195, 195, 255 });
    DrawLine(CIG_GX, CIG_GY + 1, CIG_GX + CIG_W, CIG_GY + 1, (Color){ 255, 255, 255, 255 });
    
    CIG({
      CIG_RECT(CIG_FILL),
      CIG_PARAMS({
        CIG_AXIS(CIG__HSTACK),
        CIG_SPACING(4)
      }),
      CIG_BUILDER(CIG_STACK_BUILDER)
    }) {
      CIG({
        CIG_RECT(CIG_FILL_W(54))
      }) {
        cig_enable_interaction();
        if (draw_button_panel(cig_pressed(CIG_MOUSE_BUTTON_ANY, CIG_PRESS_INSIDE))) {
          cig_label("Start", (cig_text_properties_t) {
            .alignment.horizontal = CIG_TEXT_ALIGN_CENTER,
            .alignment.vertical = CIG_TEXT_ALIGN_MIDDLE
          });
          cig_pop_frame();
        }
      }
      
      CIG({
        CIG_RECT(CIG_FILL_W(95))
      }) {
        cig_enable_interaction();
        if (draw_button_panel(cig_pressed(CIG_MOUSE_BUTTON_ANY, CIG_PRESS_INSIDE))) {
          cig_label("Welcome", (cig_text_properties_t) {
            .alignment.horizontal = CIG_TEXT_ALIGN_CENTER,
            .alignment.vertical = CIG_TEXT_ALIGN_MIDDLE
          });
          cig_pop_frame();
        }
      }
      
      CIG({
        CIG_RECT(CIG_FILL_W(95))
      }) {
        cig_enable_interaction();
        if (draw_button_panel(cig_pressed(CIG_MOUSE_BUTTON_ANY, CIG_PRESS_INSIDE))) {
          cig_label("My Computer", (cig_text_properties_t) {
            .alignment.horizontal = CIG_TEXT_ALIGN_CENTER,
            .alignment.vertical = CIG_TEXT_ALIGN_MIDDLE
          });
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
        CIG_AXIS(CIG__HSTACK),
        CIG_SPACING(2),
        CIG_ALIGNMENT_HORIZONTAL(CIG_LAYOUT_ALIGNS_RIGHT)
      }),
      CIG_BUILDER(CIG_STACK_BUILDER)
    }) {
      DrawRectangle(RECT(cig_frame()->absolute_rect), (Color){ 0, 0, 130, 255 });
      
      CIG({
        CIG_RECT(cig_rect_make(CIG_W_INSET - 16, 0, 16, CIG_FILL_CONSTANT))
      }) {
        cig_enable_interaction();
        if (draw_button_panel(cig_pressed(CIG_MOUSE_BUTTON_ANY, CIG_PRESS_INSIDE))) {
          cig_label("X", (cig_text_properties_t) {
            .font = &fonts[FONT_BOLD],
            .alignment.horizontal = CIG_TEXT_ALIGN_CENTER,
            .alignment.vertical = CIG_TEXT_ALIGN_MIDDLE
          });
          cig_pop_frame();
        }
      }
      
      cig_label("Welcome", (cig_text_properties_t) {
        .font = &fonts[FONT_BOLD],
        .color = COLOR_WHITE,
        .alignment.horizontal = CIG_TEXT_ALIGN_LEFT,
        .alignment.vertical = CIG_TEXT_ALIGN_MIDDLE
      });
    }
    
    /* Body */
    CIG({
      CIG_RECT(cig_rect_make(0, 18, CIG_FILL_CONSTANT, CIG_H_INSET - 18)),
      CIG_INSETS(cig_insets_make(15, 17, 11, 18)),
      CIG_PARAMS({
        CIG_AXIS(CIG__VSTACK),
        CIG_SPACING(12)
      }),
      CIG_BUILDER(CIG_STACK_BUILDER)
    }) {
      CIG({
        CIG_RECT(CIG_FILL_H(20))
      }) {
        cig_label("Welcome to <font=1>Windows<color=1>95</color></font>", (cig_text_properties_t) {
          .font = &fonts[FONT_TIMES_NEW_ROMAN_32_BOLD],
          .color = COLOR_BLACK,
          .alignment.horizontal = CIG_TEXT_ALIGN_LEFT,
          .alignment.vertical = CIG_TEXT_ALIGN_MIDDLE
        });
      }
      
      CIG(_) {
        CIG({
          CIG_RECT(CIG_FILL),
          CIG_INSETS(cig_insets_uniform(1)),
          CIG_PARAMS({
            CIG_AXIS(CIG__HSTACK),
            CIG_SPACING(12)
          }),
          CIG_BUILDER(CIG_STACK_BUILDER)
        }) {
          CIG({
            CIG_RECT(CIG_FILL_W(330))
          }) {
            DrawTexturePro(res.yellow_white_pattern, (Rectangle) { 0, 0, CIG_W, CIG_H }, (Rectangle) { CIG_GX + 1, CIG_GY + 1, CIG_W - 2, CIG_H - 2 }, (Vector2){0,0}, 0, WHITE);
            draw_inner_bevel_border();
          }
          
          CIG({
            CIG_RECT(CIG_FILL),
            CIG_PARAMS({
              CIG_AXIS(CIG__VSTACK),
              CIG_SPACING(6)
            }),
            CIG_BUILDER(CIG_STACK_BUILDER)
          }) {
            beveled_button(CIG_FILL_H(23), "What's New");
            beveled_button(CIG_FILL_H(23), "Online Registration");
          }
        }
      }
    }
  }
}

void render_text(cig_rect_t rect, const char *str, size_t len, cig_font_ref font, cig_text_color_ref color) {
  static char buf[24];
  strncpy(buf, str, len);
  buf[len] = '\0';
  
  struct font_store *fs = (struct font_store*)font;
  
  // DrawRectangle(RECT(rect), GREEN);
  DrawTextEx(fs->font, buf, (Vector2) { rect.x, rect.y }, fs->font.baseSize, 0, colors[color]);
}

cig_vec2_t measure_text(const char *str, size_t len, cig_font_ref font) {
  static char buf[24];
  strncpy(buf, str, len);
  buf[len] = '\0';
  
  struct font_store *fs = (struct font_store*)font;
  Vector2 bounds = MeasureTextEx(fs->font, buf, fs->font.baseSize, 0);
  
  return cig_vec2_make(bounds.x, bounds.y);
}

static cig_font_info_t font_query(cig_font_ref font_ref) {
  Font *f = (Font *)font_ref;
  return (cig_font_info_t) {
    .height = f->baseSize,
    .line_spacing = 0
  };
}