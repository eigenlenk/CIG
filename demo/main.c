#include "raylib.h"
#include "cig.h"
#include "win95/win95.h"
#include <stdio.h>
#include <string.h>

static cig_context_t ctx = { 0 };

static struct font_store {
  Font font;
  int baseline_offset, word_spacing;
} fonts[__FONT_COUNT];
static Color colors[__COLOR_COUNT];
static Texture2D images[__IMAGE_COUNT];

static void render_text(
  const char *,
  size_t,
  cig_rect_t,
  cig_font_ref,
  cig_text_color_ref,
  cig_text_style_t
);
static cig_vec2_t measure_text(
  const char *,
  size_t,
  cig_font_ref,
  cig_text_style_t
);
static cig_font_info_t font_query(cig_font_ref);

CIG_INLINED void* get_font(font_id_t font_id) {
  return &fonts[font_id];
}

CIG_INLINED void* get_color(color_id_t color_id) {
  return &colors[color_id];
}

CIG_INLINED void* get_image(image_id_t image_id) {
  return &images[image_id];
}

int main(int argc, const char *argv[]) {
  SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_ALWAYS_RUN);
  InitWindow(640, 480, "Windooze 95");
  SetTargetFPS(60);
  // ToggleFullscreen();

  images[IMAGE_BRIGHT_YELLOW_PATTERN] = LoadTexture("res/images/light_yellow_pattern.png");

  fonts[FONT_REGULAR].font = LoadFont("res/fonts/winr.fnt");
  fonts[FONT_REGULAR].baseline_offset = -2;
  fonts[FONT_REGULAR].word_spacing = 3;
  
  fonts[FONT_BOLD].font = LoadFont("res/fonts/winb.fnt");
  fonts[FONT_BOLD].baseline_offset = -2;
  fonts[FONT_BOLD].word_spacing = 3;
  
  fonts[FONT_TIMES_NEW_ROMAN_32_BOLD].font = LoadFont("res/fonts/tnr32b.fnt");
  fonts[FONT_TIMES_NEW_ROMAN_32_BOLD].baseline_offset = -6;
  fonts[FONT_TIMES_NEW_ROMAN_32_BOLD].word_spacing = 9;
  
  fonts[FONT_ARIAL_BLACK_32].font = LoadFont("res/fonts/arbl32.fnt");
  fonts[FONT_ARIAL_BLACK_32].baseline_offset = -7;
  fonts[FONT_ARIAL_BLACK_32].word_spacing = 8;
  
  fonts[FONT_FRANKLIN_GOTHIC_BOOK_32].font = LoadFont("res/fonts/gothbook32.fnt");
  fonts[FONT_FRANKLIN_GOTHIC_BOOK_32].baseline_offset = -8;
  fonts[FONT_FRANKLIN_GOTHIC_BOOK_32].word_spacing = 8;
  
  SetTextureFilter(fonts[FONT_REGULAR].font.texture, TEXTURE_FILTER_POINT);
  SetTextureFilter(fonts[FONT_BOLD].font.texture, TEXTURE_FILTER_POINT);
  SetTextureFilter(fonts[FONT_TIMES_NEW_ROMAN_32_BOLD].font.texture, TEXTURE_FILTER_POINT);
  SetTextureFilter(fonts[FONT_ARIAL_BLACK_32].font.texture, TEXTURE_FILTER_POINT);
  SetTextureFilter(fonts[FONT_FRANKLIN_GOTHIC_BOOK_32].font.texture, TEXTURE_FILTER_POINT);
  
  colors[COLOR_TEXT_BLACK] = (Color) { 0, 0, 0, 255 };
  colors[COLOR_TEXT_WHITE] = (Color) { 255, 255, 255, 255 };

  cig_init_context(&ctx);
  cig_set_text_render_callback(&render_text);
  cig_set_text_measure_callback(&measure_text);
  cig_set_font_query_callback(&font_query);
  cig_set_default_font(&fonts[FONT_REGULAR]);
  
  win95_t win_instance = {
    /* Inject dependencies */
    .get_font = &get_font,
    .get_color = &get_color,
    .get_image = &get_image
  };

  while (!WindowShouldClose()) {
    BeginDrawing();

    cig_begin_layout(&ctx, NULL, cig_rect_make(0, 0, GetScreenWidth(), GetScreenHeight()));
    
    cig_set_input_state(
      cig_vec2_make(GetMouseX(), GetMouseY()),
      IsMouseButtonDown(MOUSE_BUTTON_LEFT) ? CIG_INPUT_MOUSE_BUTTON_LEFT : 0 +
      IsMouseButtonDown(MOUSE_BUTTON_RIGHT) ? CIG_INPUT_MOUSE_BUTTON_RIGHT : 0
    );
    
    run_win95(&win_instance);
    
    cig_end_layout();

    EndDrawing();
  }

  CloseWindow();

  return 0;
}

CIG_INLINED void render_text(
  const char *str,
  size_t len,
  cig_rect_t rect,
  cig_font_ref font,
  cig_text_color_ref color,
  cig_text_style_t style
) {
  static char buf[24];
  strncpy(buf, str, len);
  buf[len] = '\0';
  
  struct font_store *fs = (struct font_store*)font;
  
  // DrawRectangle(RAYLIB_RECT(rect), GREEN);
  DrawTextEx(fs->font, buf, (Vector2) { rect.x, rect.y }, fs->font.baseSize, 0, color ? *(Color*)color : colors[COLOR_TEXT_BLACK]);
}

cig_vec2_t measure_text(
  const char *str,
  size_t len,
  cig_font_ref font,
  cig_text_style_t style
) {
  static char buf[24];
  strncpy(buf, str, len);
  buf[len] = '\0';
  
  struct font_store *fs = (struct font_store*)font;
  Vector2 bounds = MeasureTextEx(fs->font, buf, fs->font.baseSize, 0);
  
  return cig_vec2_make(bounds.x, bounds.y);
}

static cig_font_info_t font_query(cig_font_ref font_ref) {
  struct font_store *fs = (struct font_store*)font_ref;
  
  // printf("GLYPH PADDING %d\n", fs->font.glyphPadding);
  
  return (cig_font_info_t) {
    .height = fs->font.baseSize,
    .line_spacing = 0,
    .baseline_offset = fs->baseline_offset,
    .word_spacing = fs->word_spacing
  };
}