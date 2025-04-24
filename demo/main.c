#include "raylib.h"
#include "cig.h"
#include "win95/win95.h"
#include <stdio.h>
#include <string.h>

#define RAYLIB_RECT(R) R.x, R.y, R.w, R.h
#define RAYLIB_VEC2(V) (Vector2) { V.x, V.y } 

static cig_context_t ctx = { 0 };

static struct font_store {
  Font font;
  int baseline_offset, word_spacing;
} fonts[__FONT_COUNT];
static Color colors[__COLOR_COUNT];
static int panel_styles[__PANEL_COUNT];
static Texture2D images[__IMAGE_COUNT];

/* Text API */
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

/* Gfx API */
static void render_panel(cig_panel_ref, cig_rect_t, cig_panel_modifiers_t);
static void draw_rectangle(cig_color_ref, cig_color_ref, cig_rect_t, unsigned int);
static void draw_line(cig_color_ref, cig_vec2_t, cig_vec2_t, unsigned int);




CIG_INLINED void* get_font(font_id_t id) {
  return &fonts[id];
}

CIG_INLINED void* get_color(color_id_t id) {
  return &colors[id];
}

CIG_INLINED void* get_image(image_id_t id) {
  return &images[id];
}

CIG_INLINED void* get_panel(panel_id_t id) {
  return &panel_styles[id];
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
  
  colors[COLOR_BLACK] = (Color) { 0, 0, 0, 255 };
  colors[COLOR_WHITE] = (Color) { 255, 255, 255, 255 };
  colors[COLOR_DESKTOP] = (Color) { 0, 130, 130, 255 };
  colors[COLOR_DIALOG_BACKGROUND] = (Color) { 195, 195, 195, 255 };
  colors[COLOR_WINDOW_ACTIVE_TITLEBAR] = (Color) { 0, 0, 130, 255 };
  
  for (register int i = 0; i < __PANEL_COUNT; ++i) { panel_styles[i] = i; }

  cig_init_context(&ctx);
  
  cig_set_text_render_callback(&render_text);
  cig_set_text_measure_callback(&measure_text);
  cig_set_font_query_callback(&font_query);
  cig_set_default_font(&fonts[FONT_REGULAR]);
  
  cig_set_panel_render_callback(&render_panel);
  cig_set_draw_rectangle_callback(&draw_rectangle);
  cig_set_draw_line_callback(&draw_line);
  
  win95_t win_instance = {
    /* Inject dependencies */
    .get_font = &get_font,
    .get_color = &get_color,
    .get_image = &get_image,
    .get_panel = &get_panel
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
  DrawTextEx(fs->font, buf, (Vector2) { rect.x, rect.y }, fs->font.baseSize, 0, color ? *(Color*)color : colors[COLOR_BLACK]);
}

CIG_INLINED cig_vec2_t measure_text(
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

CIG_INLINED cig_font_info_t font_query(cig_font_ref font_ref) {
  struct font_store *fs = (struct font_store*)font_ref;
  
  // printf("GLYPH PADDING %d\n", fs->font.glyphPadding);
  
  return (cig_font_info_t) {
    .height = fs->font.baseSize,
    .line_spacing = 0,
    .baseline_offset = fs->baseline_offset,
    .word_spacing = fs->word_spacing
  };
}

CIG_INLINED void render_panel(cig_panel_ref panel, cig_rect_t rect, cig_panel_modifiers_t modifiers) {
  const int panel_style = *(int*)panel;
  
  switch (panel_style) {
    case PANEL_STANDARD_DIALOG: {
      DrawRectangle(RAYLIB_RECT(rect), colors[COLOR_DIALOG_BACKGROUND]);
      DrawLine(rect.x, rect.y + rect.h - 1, rect.x + rect.w, rect.y + rect.h - 1, (Color){ 0, 0, 0, 255 });
      DrawLine(rect.x + rect.w, rect.y, rect.x + rect.w, rect.y + rect.h - 1, (Color){ 0, 0, 0, 255 });
      DrawLine(rect.x + 1, rect.y + rect.h - 2, rect.x + rect.w - 1, rect.y + rect.h - 2, (Color){ 130, 130, 130, 255 });
      DrawLine(rect.x + rect.w - 1, rect.y + 1, rect.x + rect.w - 1, rect.y + rect.h - 2, (Color){ 130, 130, 130, 255 });
      DrawLine(rect.x + 1, rect.y + 1, rect.x + rect.w - 2, rect.y + 1, (Color){ 255, 255, 255, 255 });
      DrawLine(rect.x + 2, rect.y + 1, rect.x + 2, rect.y + rect.h - 2, (Color){ 255, 255, 255, 255 });
    } break;
    
    case PANEL_BUTTON: {
      DrawRectangle(RAYLIB_RECT(rect), colors[COLOR_DIALOG_BACKGROUND]);
      
      if (modifiers & CIG_PANEL_PRESSED) {
        DrawLine(rect.x, rect.y, rect.x + rect.w - 1, rect.y, (Color){ 0, 0, 0, 255 });
        DrawLine(rect.x + 1, rect.y + 1, rect.x + 1, rect.y + rect.h - 1, (Color){ 0, 0, 0, 255 });
        DrawLine(rect.x, rect.y + rect.h - 1, rect.x + rect.w, rect.y + rect.h - 1, (Color){ 255, 255, 255, 255 });
        DrawLine(rect.x + rect.w, rect.y, rect.x + rect.w, rect.y + rect.h - 1, (Color){ 255, 255, 255, 255 }); 
        DrawLine(rect.x + 1, rect.y + rect.h - 2, rect.x + rect.w - 1, rect.y + rect.h - 2, (Color){ 223, 223, 223, 255 });
        DrawLine(rect.x + rect.w - 1, rect.y + 1, rect.x + rect.w - 1, rect.y + rect.h - 2, (Color){ 223, 223, 223, 255 });
        DrawLine(rect.x + 2, rect.y + 1, rect.x + rect.w - 2, rect.y + 1, (Color){ 128, 128, 128, 255 });
        DrawLine(rect.x + 2, rect.y + 1, rect.x + 2, rect.y + rect.h - 2, (Color){ 128, 128, 128, 255 });
      } else {
        DrawLine(rect.x, rect.y, rect.x + rect.w - 1, rect.y, (Color){ 255, 255, 255, 255 });
        DrawLine(rect.x + 1, rect.y + 1, rect.x + 1, rect.y + rect.h - 1, (Color){ 255, 255, 255, 255 });
        DrawLine(rect.x, rect.y + rect.h - 1, rect.x + rect.w, rect.y + rect.h - 1, (Color){ 0, 0, 0, 255 });
        DrawLine(rect.x + rect.w, rect.y, rect.x + rect.w, rect.y + rect.h - 1, (Color){ 0, 0, 0, 255 });
        DrawLine(rect.x + 1, rect.y + rect.h - 2, rect.x + rect.w - 1, rect.y + rect.h - 2, (Color){ 130, 130, 130, 255 });
        DrawLine(rect.x + rect.w - 1, rect.y + 1, rect.x + rect.w - 1, rect.y + rect.h - 2, (Color){ 130, 130, 130, 255 });
      }
    } break;
    
    case PANEL_LIGHT_YELLOW: {
      DrawTexturePro(images[IMAGE_BRIGHT_YELLOW_PATTERN], (Rectangle) { 0, 0, rect.w, rect.h }, (Rectangle) { rect.x + 1, rect.y + 1, rect.w - 2, rect.h - 2 }, (Vector2){ 0, 0 }, 0, WHITE);
    } break;
    
    case PANEL_INNER_BEVEL_NO_FILL: {
      DrawLine(rect.x, rect.y, rect.x + rect.w - 1, rect.y, (Color) { 130, 130, 130, 255 });
      DrawLine(rect.x + 1, rect.y, rect.x + 1, rect.y + rect.h - 1, (Color) { 130, 130, 130, 255 });
      DrawLine(rect.x, rect.y + rect.h - 1, rect.x + rect.w , rect.y + rect.h - 1, (Color) { 255, 255, 255, 255 });
      DrawLine(rect.x + rect.w , rect.y, rect.x + rect.w , rect.y + rect.h, (Color) { 255, 255, 255, 255 });
    } break;
  }
}

CIG_INLINED void draw_rectangle(
  cig_color_ref fill_color,
  cig_color_ref border_color,
  cig_rect_t rect,
  unsigned int border_width
) {
  DrawRectangle(RAYLIB_RECT(rect), *(Color*)fill_color);
}

CIG_INLINED void draw_line(
  cig_color_ref color,
  cig_vec2_t p0,
  cig_vec2_t p1,
  unsigned int thickness
) {
  DrawLineEx(RAYLIB_VEC2(p0), RAYLIB_VEC2(p1), thickness, *(Color*)color);
}