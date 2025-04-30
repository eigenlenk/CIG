#include "raylib.h"
#include "cig.h"
#include "win95/win95.h"
#include <stdio.h>
#include <string.h>

#define UNPACK_RECT(R) R.x, R.y, R.w, R.h
#define RAYLIB_RECT(R) (Rectangle ) { R.x, R.y, R.w, R.h } 
#define RAYLIB_VEC2(V) (Vector2) { V.x, V.y } 

static cig_context_t ctx = { 0 };

static struct font_store {
  Font font;
  int baseline_offset, word_spacing;
} fonts[__FONT_COUNT];
static Color colors[__COLOR_COUNT];
static int panel_styles[__PANEL_COUNT];
static Texture2D images[__IMAGE_COUNT];

/* Core API */
static void set_clip_rect(cig_buffer_ref, cig_rect_t, bool);

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
static void draw_image(cig_buffer_ref, cig_rect_t, cig_image_ref, cig_image_mode_t);
static void render_panel(cig_panel_ref, cig_rect_t, cig_panel_modifiers_t);
static void draw_rectangle(cig_color_ref, cig_color_ref, cig_rect_t, unsigned int);
static void draw_line(cig_color_ref, cig_vec2_t, cig_vec2_t, float);

void* get_font(font_id_t id) {
  return &fonts[id];
}

void* get_color(color_id_t id) {
  return &colors[id];
}

void* get_image(image_id_t id) {
  return &images[id];
}

void* get_panel(panel_id_t id) {
  return &panel_styles[id];
}

CIG_INLINED void load_texture(Texture2D *dst, const char *path) {
  *dst = LoadTexture(path);
  SetTextureFilter(*dst, TEXTURE_FILTER_POINT);
}

int main(int argc, const char *argv[]) {
// SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
  SetConfigFlags(FLAG_WINDOW_ALWAYS_RUN);
  InitWindow(1280, 960, "Windooze 95");
  SetTargetFPS(60);
  // ToggleFullscreen();

  load_texture(&images[IMAGE_BRIGHT_YELLOW_PATTERN], "res/images/light_yellow_pattern.png");
  load_texture(&images[IMAGE_GRAY_DITHER], "res/images/gray_dither.png");
  load_texture(&images[IMAGE_START_ICON], "res/images/start.png");
  load_texture(&images[IMAGE_MY_COMPUTER_16], "res/images/my_computer.png");
  load_texture(&images[IMAGE_MY_COMPUTER_32], "res/images/my_computer_32.png");
  load_texture(&images[IMAGE_TIP_OF_THE_DAY], "res/images/tip_of_the_day.png");
  load_texture(&images[IMAGE_CHECKMARK], "res/images/check.png");
  load_texture(&images[IMAGE_WELCOME_APP_ICON], "res/images/welcome.png");
  load_texture(&images[IMAGE_BIN_EMPTY], "res/images/bin_empty.png");

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

  cig_set_clip_rect_callback(&set_clip_rect);
  
  cig_set_text_render_callback(&render_text);
  cig_set_text_measure_callback(&measure_text);
  cig_set_font_query_callback(&font_query);

  cig_set_default_font(&fonts[FONT_REGULAR]);
  cig_set_default_text_color(&colors[COLOR_BLACK]);

  cig_set_draw_image_callback(&draw_image);
  cig_set_panel_render_callback(&render_panel);
  cig_set_draw_rectangle_callback(&draw_rectangle);
  cig_set_draw_line_callback(&draw_line);
  
  cig_begin_layout(&ctx, NULL, cig_rect_make(0, 0, 640, 480), 0.f);

  win95_t win_instance = { 0 };
  start_win95(&win_instance);

  RenderTexture2D render_texture = LoadRenderTexture(640, 480);
  SetTextureFilter(render_texture.texture, TEXTURE_FILTER_POINT);

  while (!WindowShouldClose()) {
    BeginDrawing();

    BeginTextureMode(render_texture);
    cig_begin_layout(&ctx, NULL, cig_rect_make(0, 0, 640, 480), GetFrameTime());
    
    cig_set_input_state(
      cig_vec2_make(GetMouseX()/2, GetMouseY()/2),
      IsMouseButtonDown(MOUSE_BUTTON_LEFT) ? CIG_INPUT_MOUSE_BUTTON_LEFT : 0 +
      IsMouseButtonDown(MOUSE_BUTTON_RIGHT) ? CIG_INPUT_MOUSE_BUTTON_RIGHT : 0
    );
    
    run_win95(&win_instance);
    
    cig_end_layout();

    EndTextureMode();

    DrawTexturePro(
      render_texture.texture,
      (Rectangle) { 0, 0, (float)render_texture.texture.width, (float)-render_texture.texture.height },
      (Rectangle) { 0, 0, GetScreenWidth(), GetScreenHeight() },
      (Vector2) { 0, 0 },
      0,
      WHITE
    );

    EndDrawing();
  }

  UnloadRenderTexture(render_texture); 
  CloseWindow();

  return 0;
}



CIG_INLINED void set_clip_rect(cig_buffer_ref buffer, cig_rect_t rect, bool is_root) {
  if (is_root) {
    EndScissorMode();
  } else {
    BeginScissorMode(UNPACK_RECT(rect));
  }
  // printf("set_clip_rect: %d, %d, %d, %d (%d)\n", rect.x, rect.y, rect.w, rect.h, is_root);
}

CIG_INLINED void render_text(
  const char *str,
  size_t len,
  cig_rect_t rect,
  cig_font_ref font,
  cig_text_color_ref color,
  cig_text_style_t style
) {
  static char buf[256];
  strncpy(buf, str, len);
  buf[len] = '\0';
  
  struct font_store *fs = (struct font_store*)font;

  // printf("render: |%s|\n", buf);
  
  // DrawRectangle(UNPACK_RECT(rect), GREEN);
  DrawTextEx(fs->font, buf, (Vector2) { rect.x, rect.y }, fs->font.baseSize, 0, *(Color*)color);
}

CIG_INLINED cig_vec2_t measure_text(
  const char *str,
  size_t len,
  cig_font_ref font,
  cig_text_style_t style
) {
  static char buf[256];
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
      DrawRectangle(UNPACK_RECT(rect), colors[COLOR_DIALOG_BACKGROUND]);

      DrawLine(rect.x, rect.y + rect.h - 1, rect.x + rect.w, rect.y + rect.h - 1, (Color) { 0, 0, 0, 255 });
      DrawLine(rect.x + rect.w, rect.y, rect.x + rect.w, rect.y + rect.h - 1, (Color) { 0, 0, 0, 255 });
      DrawLine(rect.x + 1, rect.y + rect.h - 2, rect.x + rect.w - 1, rect.y + rect.h - 2, (Color) { 130, 130, 130, 255 });
      DrawLine(rect.x + rect.w - 1, rect.y + 1, rect.x + rect.w - 1, rect.y + rect.h - 2, (Color) { 130, 130, 130, 255 });
      DrawLine(rect.x + 1, rect.y + 1, rect.x + rect.w - 2, rect.y + 1, (Color) { 255, 255, 255, 255 });
      DrawLine(rect.x + 2, rect.y + 1, rect.x + 2, rect.y + rect.h - 2, (Color) { 255, 255, 255, 255 });
    } break;
    
    case PANEL_BUTTON: {
      if (modifiers & CIG_PANEL_SELECTED) {
        DrawTexturePro(images[IMAGE_GRAY_DITHER], (Rectangle) { 0, 0, rect.w-4, rect.h-5 }, (Rectangle) { rect.x+2, rect.y+3, rect.w-4, rect.h-5 }, (Vector2){ 0, 0 }, 0, WHITE);
        DrawLine(rect.x, rect.y, rect.x + rect.w - 1, rect.y, (Color){ 0, 0, 0, 255 });
        DrawLine(rect.x + 1, rect.y + 1, rect.x + 1, rect.y + rect.h - 1, (Color){ 0, 0, 0, 255 });
        DrawLine(rect.x, rect.y + rect.h - 1, rect.x + rect.w, rect.y + rect.h - 1, (Color){ 255, 255, 255, 255 });
        DrawLine(rect.x + rect.w, rect.y, rect.x + rect.w, rect.y + rect.h - 1, (Color){ 255, 255, 255, 255 }); 
        DrawLine(rect.x + 1, rect.y + rect.h - 2, rect.x + rect.w - 1, rect.y + rect.h - 2, (Color){ 223, 223, 223, 255 });
        DrawLine(rect.x + rect.w - 1, rect.y + 1, rect.x + rect.w - 1, rect.y + rect.h - 2, (Color){ 223, 223, 223, 255 });
        DrawLine(rect.x + 2, rect.y + 1, rect.x + rect.w - 2, rect.y + 1, (Color){ 128, 128, 128, 255 });
        DrawLine(rect.x + 2, rect.y + 1, rect.x + 2, rect.y + rect.h - 2, (Color){ 128, 128, 128, 255 });
        DrawLine(rect.x + 2, rect.y + 2, rect.x + rect.w - 2, rect.y + 2, (Color){ 255, 255, 255, 255 });
      } else if (modifiers & CIG_PANEL_PRESSED) {
        DrawRectangle(UNPACK_RECT(rect), colors[COLOR_DIALOG_BACKGROUND]);
        DrawLine(rect.x, rect.y, rect.x + rect.w - 1, rect.y, (Color){ 0, 0, 0, 255 });
        DrawLine(rect.x + 1, rect.y + 1, rect.x + 1, rect.y + rect.h - 1, (Color){ 0, 0, 0, 255 });
        DrawLine(rect.x, rect.y + rect.h - 1, rect.x + rect.w, rect.y + rect.h - 1, (Color){ 255, 255, 255, 255 });
        DrawLine(rect.x + rect.w, rect.y, rect.x + rect.w, rect.y + rect.h - 1, (Color){ 255, 255, 255, 255 }); 
        DrawLine(rect.x + 1, rect.y + rect.h - 2, rect.x + rect.w - 1, rect.y + rect.h - 2, (Color){ 223, 223, 223, 255 });
        DrawLine(rect.x + rect.w - 1, rect.y + 1, rect.x + rect.w - 1, rect.y + rect.h - 2, (Color){ 223, 223, 223, 255 });
        DrawLine(rect.x + 2, rect.y + 1, rect.x + rect.w - 2, rect.y + 1, (Color){ 128, 128, 128, 255 });
        DrawLine(rect.x + 2, rect.y + 1, rect.x + 2, rect.y + rect.h - 2, (Color){ 128, 128, 128, 255 });
      } else {
        DrawRectangle(UNPACK_RECT(rect), colors[COLOR_DIALOG_BACKGROUND]);
        DrawLine(rect.x, rect.y, rect.x + rect.w - 1, rect.y, (Color){ 255, 255, 255, 255 });
        DrawLine(rect.x + 1, rect.y + 1, rect.x + 1, rect.y + rect.h - 1, (Color){ 255, 255, 255, 255 });
        DrawLine(rect.x, rect.y + rect.h - 1, rect.x + rect.w, rect.y + rect.h - 1, (Color){ 0, 0, 0, 255 });
        DrawLine(rect.x + rect.w, rect.y, rect.x + rect.w, rect.y + rect.h - 1, (Color){ 0, 0, 0, 255 });
        DrawLine(rect.x + 1, rect.y + rect.h - 2, rect.x + rect.w - 1, rect.y + rect.h - 2, (Color){ 130, 130, 130, 255 });
        DrawLine(rect.x + rect.w - 1, rect.y + 1, rect.x + rect.w - 1, rect.y + rect.h - 2, (Color){ 130, 130, 130, 255 });
      }
    } break;
    
    case PANEL_LIGHT_YELLOW: {
      DrawTexturePro(images[IMAGE_BRIGHT_YELLOW_PATTERN], (Rectangle) { 0, 0, rect.w, rect.h }, (Rectangle) { rect.x, rect.y, rect.w, rect.h }, (Vector2){ 0, 0 }, 0, WHITE);
    } break;

    case PANEL_GRAY_DITHER: {
      DrawTexturePro(images[IMAGE_GRAY_DITHER], (Rectangle) { 0, 0, rect.w, rect.h }, (Rectangle) { rect.x, rect.y, rect.w, rect.h }, (Vector2){ 0, 0 }, 0, WHITE);
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
  if (fill_color) {
    DrawRectangle(UNPACK_RECT(rect), *(Color*)fill_color);
  }
  if (border_color) {
    DrawRectangleLinesEx(RAYLIB_RECT(rect), border_width, *(Color*)border_color);
  }
}

CIG_INLINED void draw_line(
  cig_color_ref color,
  cig_vec2_t p0,
  cig_vec2_t p1,
  float thickness
) {
  DrawLineEx(RAYLIB_VEC2(p0), RAYLIB_VEC2(p1), thickness, *(Color*)color);
}

CIG_INLINED void draw_image(
  cig_buffer_ref buffer,
  cig_rect_t rect,
  cig_image_ref image,
  cig_image_mode_t mode
) {
  Texture2D *tex = (Texture2D *)image;

  if (mode == CIG_IMAGE_MODE_ASPECT_FIT) {
    const float srcAspect = tex->width / tex->height;
    const float dstAspect = (float)rect.w / rect.h;
    const float scale = (srcAspect > dstAspect)
        ? ((float)rect.w / tex->width)
        : ((float)rect.h / tex->height);
    DrawTextureEx(
      *tex,
      (Vector2) {
        rect.x+(rect.w-tex->width*scale)*0.5,
        rect.y+(rect.h-tex->height*scale)*0.5
      },
      0,
      scale,
      WHITE
    );
  } else if (mode == CIG_IMAGE_MODE_ASPECT_FILL) {
    const float srcAspect = tex->width / tex->height;
    const float dstAspect = (float)rect.w / rect.h;
    const float scale = (srcAspect < dstAspect)
        ? ((float)rect.w / tex->width)
        : ((float)rect.h / tex->height);
    DrawTextureEx(
      *tex,
      (Vector2) {
        rect.x+(rect.w-tex->width*scale)*0.5,
        rect.y+(rect.h-tex->height*scale)*0.5
      },
      0,
      scale,
      WHITE
    );
  } else if (mode == CIG_IMAGE_MODE_SCALE_TO_FILL) {
    DrawTexturePro(
      *tex,
      (Rectangle) { 0, 0, tex->width, tex->height },
      RAYLIB_RECT(rect),
      (Vector2) { 0, 0 },
      0,
      WHITE
    );
  } else if (mode >= CIG_IMAGE_MODE_CENTER) {
    Vector2 positions[9] = {
      { 0.5, 0.5 }, /* CIG_IMAGE_MODE_CENTER */
      { 0.0, 0.5 }, /* CIG_IMAGE_MODE_LEFT */
      { 1.0, 0.5 }, /* CIG_IMAGE_MODE_RIGHT */
      { 0.5, 0.0 }, /* CIG_IMAGE_MODE_TOP */
      { 0.5, 1.0 }, /* CIG_IMAGE_MODE_BOTTOM */
      { 0.0, 0.0 }, /* CIG_IMAGE_MODE_TOP_LEFT */
      { 1.0, 0.0 }, /* CIG_IMAGE_MODE_TOP_RIGHT */
      { 0.0, 1.0 }, /* CIG_IMAGE_MODE_BOTTOM_LEFT */
      { 1.0, 1.0 }, /* CIG_IMAGE_MODE_BOTTOM_RIGHT */
    };
    DrawTexture(
      *tex,
      rect.x+(rect.w-tex->width)*positions[mode-CIG_IMAGE_MODE_CENTER].x,
      rect.y+(rect.h-tex->height)*positions[mode-CIG_IMAGE_MODE_CENTER].y,
      WHITE
    ); 
  }
}