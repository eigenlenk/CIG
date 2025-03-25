#include "raylib.h"
#include "cigcore.h"

#define RECT(R) R.x, R.y, R.w, R.h

static cig_context_t ctx = { 0 };

static void demo_ui();

int main(int argc, const char *argv[]) {
  InitWindow(800, 600, "CIG Demo");
  SetTargetFPS(60);

  cig_init_context(&ctx);

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
  CIG_ARRANGE(rect, CIG_BODY(
    cig_enable_interaction();
    DrawRectangle(
      RECT((cig_pressed(CIG_MOUSE_BUTTON_ANY, CIG_PRESS_INSIDE) ? cig_rect_offset(cig_frame()->absolute_rect, 0, 2) : cig_frame()->absolute_rect)),
      cig_hovered() ? PURPLE : BLUE
    );
    clicked = cig_clicked(CIG_MOUSE_BUTTON_ANY, CIG_CLICK_STARTS_INSIDE);
  ))
  return clicked;
}

static void demo_ui() {
  DrawRectangle(RECT(cig_frame()->absolute_rect), DARKBLUE);
  
  if (cig_push_layout_function(&cig_default_layout_builder, CIG_FILL, cig_insets_uniform(10), (cig_layout_params_t) { 0,
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
  }
}