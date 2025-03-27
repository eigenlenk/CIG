#include "raylib.h"
#include "cig.h"
#include <stdio.h>
#include <string.h>

#define RECT(R) R.x, R.y, R.w, R.h
#define TASKBAR_FONT_SIZE 14

static cig_context_t ctx = { 0 };
static Font font;

static void demo_ui();
static void render_text(cig_rect_t, const char*, size_t);
static cig_vec2_t measure_text(const char*, size_t);

int main(int argc, const char *argv[]) {
  SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_ALWAYS_RUN);
  InitWindow(640, 480, "CIG Demo");
  SetTargetFPS(60);
  
  font = LoadFontEx("micross.ttf", TASKBAR_FONT_SIZE, 0, 0);

  cig_init_context(&ctx);
  cig_set_text_render_callback(&render_text);
  cig_set_text_measure_callback(&measure_text);

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

static bool clickable_box(cig_rect_t rect) {
  bool clicked = false;
  
  CIG_ARRANGE(
    rect,
    CIG_BODY(
      cig_enable_interaction();
      
      clicked = cig_clicked(CIG_MOUSE_BUTTON_ANY, CIG_CLICK_STARTS_INSIDE);
      bool pressed = cig_pressed(CIG_MOUSE_BUTTON_ANY, CIG_PRESS_INSIDE);
      
      CIG_ARRANGE(
        pressed ? cig_rect_make(0, 2, CIG_W, CIG_H) : CIG_FILL,
        CIG_BODY(
          DrawRectangle(RECT(cig_absolute_rect()), pressed ? PURPLE : BLUE);
          cig_label("Hi! Olá mundo!");
        )
      )
    )
  )
  
  return clicked;
}

#define TASKBAR_H 28

static void draw_win32_panel(bool pressed) {
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
    
    CIG_HSTACK(
      CIG_FILL,
      CIG_BODY(
        CIG_ARRANGE(CIG_FILL_W(54), CIG_BODY(
          cig_enable_interaction();
          draw_win32_panel(cig_pressed(CIG_MOUSE_BUTTON_ANY, CIG_PRESS_INSIDE));
          cig_label("Start");
        ))
        
        CIG_ARRANGE(CIG_FILL_W(95), CIG_BODY(
          cig_enable_interaction();
          draw_win32_panel(cig_pressed(CIG_MOUSE_BUTTON_ANY, CIG_PRESS_INSIDE));
          cig_label("Welcome");
        ))
        
        CIG_ARRANGE(CIG_FILL_W(95), CIG_BODY(
          cig_enable_interaction();
          draw_win32_panel(cig_pressed(CIG_MOUSE_BUTTON_ANY, CIG_PRESS_INSIDE));
          cig_label("My Computer");
        ))
      ),
      CIG_SPACING(4)
    )

    cig_pop_frame();
  }
  
  
  
  
  
  
  /*if (cig_push_layout_function(&cig_default_layout_builder, CIG_FILL, cig_insets_uniform(10), (cig_layout_params_t) { 0,
    .axis = CIG_LAYOUT_AXIS_VERTICAL,
    .height = 50,
    .spacing = 10
  })) {
    for (int i = 0; i < 8; ++i) {
      clickable_box(CIG_FILL);
    }
    
    cig_frame()->_layout_params.height = 0;
    
    CIG_HSTACK(CIG_FILL, CIG_BODY(
      for (int i = 0; i < 10; ++i) {
        clickable_box(CIG_FILL);
      }
    ), CIG_COLUMNS(10), CIG_SPACING(10))
    
    cig_pop_frame();
  }*/
}

void render_text(cig_rect_t rect, const char *str, size_t len) {
  static char buf[24];
  strncpy(buf, str, len);
  buf[len] = '\0';
  
  // DrawRectangle(RECT(rect), RED);
  DrawTextEx(font, buf, (Vector2) { rect.x, rect.y }, TASKBAR_FONT_SIZE, 0, (Color) { 0, 0, 0, 255 });
}

cig_vec2_t measure_text(const char *str, size_t len) {
  static char buf[24];
  strncpy(buf, str, len);
  buf[len] = '\0';
  
  Vector2 bounds = MeasureTextEx(font, buf, TASKBAR_FONT_SIZE, 0);
  
  return cig_vec2_make(bounds.x, bounds.y);
}