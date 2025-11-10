#include "raylib.h"
#include "cig.h"
#include "win95.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#define UNPACK_RECT(R) R.x, R.y, R.w, R.h
#define RAYLIB_RECT(R) (Rectangle ) { R.x, R.y, R.w, R.h } 
#define RAYLIB_VEC2(V) (Vector2) { V.x, V.y } 

static cig_context ctx = { 0 };

static struct font_store {
  Font font;
  int baseline_offset;
} fonts[__FONT_COUNT];
static Color colors[__COLOR_COUNT];
static int panel_styles[__STYLE_COUNT];
static Texture2D images[__IMAGE_COUNT];
static Shader blue_dither_shader;
static bool dithering_shader_enabled = false;
static RenderTexture2D render_texture;

/*  Core API */
static void set_clip_rect(cig_buffer_ref, cig_r, bool);

/*  Text API */
static void render_text(
  const char *,
  size_t,
  cig_r,
  cig_font_ref,
  cig_text_color_ref,
  cig_text_style
);
static cig_v measure_text(
  const char *,
  size_t,
  cig_font_ref,
  cig_text_style
);
static cig_font_info_st font_query(cig_font_ref);

/* Gfx API */
static void draw_image(cig_buffer_ref, cig_r, cig_r, cig_image_ref, cig_image_mode);
static cig_v measure_image(cig_image_ref);
static void draw_style(cig_style_ref, cig_r, cig_style_modifiers);
static void draw_rectangle(cig_color_ref, cig_color_ref, cig_r, unsigned int);
static void draw_line(cig_color_ref, cig_v, cig_v, float);

#ifdef DEBUG
static RenderTexture2D debug_texture;
static void layout_breakpoint(cig_r, cig_r);
#endif

void* get_font(font_id_t id) {
  return &fonts[id];
}

void* get_color(color_id_t id) {
  return &colors[id];
}

void* get_image(image_id_t id) {
  return &images[id];
}

void* get_style(style_id_t id) {
  return &panel_styles[id];
}

void enable_blue_selection_dithering(bool enabled) {
  dithering_shader_enabled = enabled;
}

CIG_INLINED void load_texture(Texture2D *dst, const char *path) {
  *dst = LoadTexture(path);
  SetTextureFilter(*dst, TEXTURE_FILTER_POINT);
}

static int win95_w, win95_h;
static double scale;

static void
set_up_render_textures()
{
  render_texture = LoadRenderTexture(win95_w, win95_h);
  SetTextureFilter(render_texture.texture, TEXTURE_FILTER_POINT);

#ifdef DEBUG
  debug_texture = LoadRenderTexture(render_texture.texture.width, render_texture.texture.height);
  SetTextureFilter(render_texture.texture, TEXTURE_FILTER_POINT);
#endif
}

static void
check_alt_enter()
{
  if (IsKeyPressed(KEY_ENTER) && (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT))) {
    int ray_w, ray_h;
    if (IsWindowFullscreen()) {
      ray_w = 1280;
      ray_h = 960;
    } else {
      int display = GetCurrentMonitor(); 
      ray_w = GetMonitorWidth(display);
      ray_h = GetMonitorHeight(display);
    }
    win95_w = ray_w * scale;
    win95_h = ray_h * scale;

    ToggleFullscreen();
    SetWindowSize(ray_w, ray_h);

    set_up_render_textures();

    cig_begin_layout(&ctx, NULL, cig_r_make(0, 0, win95_w, win95_h), 0.f);
    win95_did_change_resolution();
  }
}

int main(int argc, const char *argv[]) {
  srand(time(NULL));

  bool run_fullscreen = false;
  int i, ray_w, ray_h;

  for (i = 1; i < argc; ++i) {
    if (!strcmp("-f", argv[i])) {
      run_fullscreen = true;
    }
  }

  SetTraceLogLevel(LOG_WARNING);

  if (run_fullscreen) {
    InitWindow(0, 0, "Winlose 95");
    ray_w = GetMonitorWidth(GetCurrentMonitor());
    ray_h = GetMonitorHeight(GetCurrentMonitor());
    scale = 0.5;
  } else {
    ray_w = 1280;
    ray_h = 960;
    scale = 0.5;
  }

  win95_w = ray_w * scale;
  win95_h = ray_h * scale;

  // SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
  SetConfigFlags(FLAG_WINDOW_ALWAYS_RUN);
  InitWindow(ray_w, ray_h, "Winlose 95");
  SetTargetFPS(60);

  if (run_fullscreen) {
    ToggleFullscreen();
  }

  // TODO: Would be neater to package multiple sizes and reference these simply as "name@16" or something
  load_texture(&images[IMAGE_BRIGHT_YELLOW_PATTERN], "res/images/light_yellow_pattern.png");
  load_texture(&images[IMAGE_GRAY_DITHER], "res/images/gray_dither.png");
  load_texture(&images[IMAGE_START_ICON], "res/images/start.png");
  load_texture(&images[IMAGE_LOGO_TEXT], "res/images/logotext.png");
  load_texture(&images[IMAGE_START_SIDEBAR], "res/images/start_sidebar.png");
  load_texture(&images[IMAGE_MY_COMPUTER_16], "res/images/my_computer.png");
  load_texture(&images[IMAGE_MY_COMPUTER_32], "res/images/my_computer_32.png");
  load_texture(&images[IMAGE_TIP_OF_THE_DAY], "res/images/tip_of_the_day.png");
  load_texture(&images[IMAGE_CHECKMARK], "res/images/check.png");
  load_texture(&images[IMAGE_CROSS], "res/images/cross.png");
  load_texture(&images[IMAGE_MAXIMIZE], "res/images/maximize.png");
  load_texture(&images[IMAGE_MINIMIZE], "res/images/minimize.png");
  load_texture(&images[IMAGE_RESTORE], "res/images/restore.png");
  load_texture(&images[IMAGE_SCROLL_UP], "res/images/scroll_up.png");
  load_texture(&images[IMAGE_SCROLL_DOWN], "res/images/scroll_down.png");
  load_texture(&images[IMAGE_SCROLL_LEFT], "res/images/scroll_left.png");
  load_texture(&images[IMAGE_SCROLL_RIGHT], "res/images/scroll_right.png");
  load_texture(&images[IMAGE_WELCOME_APP_ICON], "res/images/welcome.png");
  load_texture(&images[IMAGE_BIN_EMPTY], "res/images/bin_empty.png");
  load_texture(&images[IMAGE_BIN_EMPTY_16], "res/images/bin_16.png");
  load_texture(&images[IMAGE_DRIVE_A_16], "res/images/drive_a_16.png");
  load_texture(&images[IMAGE_DRIVE_A_32], "res/images/drive_a_32.png");
  load_texture(&images[IMAGE_DRIVE_C_16], "res/images/drive_c_16.png");
  load_texture(&images[IMAGE_DRIVE_C_32], "res/images/drive_c_32.png");
  load_texture(&images[IMAGE_DRIVE_D_16], "res/images/drive_d_16.png");
  load_texture(&images[IMAGE_DRIVE_D_32], "res/images/drive_d_32.png");
  load_texture(&images[IMAGE_CONTROLS_FOLDER_16], "res/images/controls_folder_16.png");
  load_texture(&images[IMAGE_CONTROLS_FOLDER_32], "res/images/controls_folder_32.png");
  load_texture(&images[IMAGE_PRINTERS_FOLDER_16], "res/images/printers_folder_16.png");
  load_texture(&images[IMAGE_PRINTERS_FOLDER_32], "res/images/printers_folder_32.png");
  load_texture(&images[IMAGE_DIAL_UP_FOLDER_16], "res/images/dial_up_folder_16.png");
  load_texture(&images[IMAGE_DIAL_UP_FOLDER_32], "res/images/dial_up_folder_32.png");
  load_texture(&images[IMAGE_RESIZE_HANDLE], "res/images/resize_handle.png");
  load_texture(&images[IMAGE_MENU_CHECK], "res/images/menu_check.png");
  load_texture(&images[IMAGE_MENU_CHECK_INVERTED], "res/images/menu_check_inverted.png");
  load_texture(&images[IMAGE_MENU_RADIO], "res/images/menu_radio.png");
  load_texture(&images[IMAGE_MENU_RADIO_INVERTED], "res/images/menu_radio_inverted.png");
  load_texture(&images[IMAGE_MENU_ARROW], "res/images/menu_arrow.png");
  load_texture(&images[IMAGE_MENU_ARROW_INVERTED], "res/images/menu_arrow_inverted.png");
  load_texture(&images[IMAGE_PROGRAM_FOLDER_24], "res/images/program_folder_24.png");
  load_texture(&images[IMAGE_PROGRAM_FOLDER_16], "res/images/program_folder_16.png");
  load_texture(&images[IMAGE_DOCUMENTS_24], "res/images/documents_24.png");
  load_texture(&images[IMAGE_SETTINGS_24], "res/images/settings_24.png");
  load_texture(&images[IMAGE_FIND_24], "res/images/find_24.png");
  load_texture(&images[IMAGE_HELP_24], "res/images/help_24.png");
  load_texture(&images[IMAGE_RUN_24], "res/images/run_24.png");
  load_texture(&images[IMAGE_SHUT_DOWN_24], "res/images/shut_down_24.png");
  load_texture(&images[IMAGE_MAIL_16], "res/images/mail_16.png");
  load_texture(&images[IMAGE_MSDOS_16], "res/images/msdos_16.png");
  load_texture(&images[IMAGE_MSN_16], "res/images/msn_16.png");
  load_texture(&images[IMAGE_EXPLORER_16], "res/images/explorer_16.png");
  load_texture(&images[IMAGE_CALCULATOR_16], "res/images/calculator_16.png");
  load_texture(&images[IMAGE_NOTEPAD_16], "res/images/notepad_16.png");
  load_texture(&images[IMAGE_PAINT_16], "res/images/paint_16.png");
  load_texture(&images[IMAGE_WORDWIZ_16], "res/images/wordwiz_16.png");

  blue_dither_shader = LoadShader(0, "res/shaders/blue_dither.fs");

  fonts[FONT_REGULAR].font = LoadFont("res/fonts/winr.fnt");
  fonts[FONT_REGULAR].baseline_offset = -2;
  
  fonts[FONT_BOLD].font = LoadFont("res/fonts/winb.fnt");
  fonts[FONT_BOLD].baseline_offset = -2;
  
  fonts[FONT_TIMES_NEW_ROMAN_32_BOLD].font = LoadFont("res/fonts/tnr32b.fnt");
  fonts[FONT_TIMES_NEW_ROMAN_32_BOLD].baseline_offset = -6;
  
  fonts[FONT_ARIAL_BLACK_32].font = LoadFont("res/fonts/arbl32.fnt");
  fonts[FONT_ARIAL_BLACK_32].baseline_offset = -7;
  
  fonts[FONT_FRANKLIN_GOTHIC_BOOK_32].font = LoadFont("res/fonts/gothbook32.fnt");
  fonts[FONT_FRANKLIN_GOTHIC_BOOK_32].baseline_offset = -8;

  SetTextureFilter(fonts[FONT_REGULAR].font.texture, TEXTURE_FILTER_POINT);
  SetTextureFilter(fonts[FONT_BOLD].font.texture, TEXTURE_FILTER_POINT);
  SetTextureFilter(fonts[FONT_TIMES_NEW_ROMAN_32_BOLD].font.texture, TEXTURE_FILTER_POINT);
  SetTextureFilter(fonts[FONT_ARIAL_BLACK_32].font.texture, TEXTURE_FILTER_POINT);
  SetTextureFilter(fonts[FONT_FRANKLIN_GOTHIC_BOOK_32].font.texture, TEXTURE_FILTER_POINT);
  
  colors[COLOR_DEBUG] = (Color) { 255, 0, 255, 255 };
  colors[COLOR_BLACK] = (Color) { 0, 0, 0, 255 };
  colors[COLOR_WHITE] = (Color) { 255, 255, 255, 255 };
  colors[COLOR_YELLOW] = (Color) { 255, 255, 0, 255 };
  colors[COLOR_GREEN] = (Color) { 0, 128, 0, 255 };
  colors[COLOR_RED] = (Color) { 255, 0, 0, 255 };
  colors[COLOR_MAROON] = (Color) { 128, 0, 0, 255 };
  colors[COLOR_BLUE] = (Color) { 0, 0, 255, 255 };
  colors[COLOR_NAVY] = (Color) { 0, 0, 128, 255 };
  colors[COLOR_DESKTOP_BG] = (Color) { 0, 127, 127, 255 };
  colors[COLOR_DIALOG_BACKGROUND] = (Color) { 195, 195, 195, 255 };
  colors[COLOR_WINDOW_ACTIVE_TITLEBAR] = (Color) { 0, 0, 127, 255 };
  colors[COLOR_WINDOW_INACTIVE_TITLEBAR] = (Color) { 127, 127, 127, 255 };
  
  for (i = 0; i < __STYLE_COUNT; ++i) { panel_styles[i] = i; }

  cig_init_context(&ctx);

  cig_assign_set_clip(&set_clip_rect);
  
  cig_assign_draw_text(&render_text);
  cig_assign_measure_text(&measure_text);
  cig_assign_query_font(&font_query);

  cig_set_default_font(&fonts[FONT_REGULAR]);
  cig_set_default_text_color(&colors[COLOR_BLACK]);

  cig_assign_draw_image(&draw_image);
  cig_assign_measure_image(&measure_image);
  cig_assign_draw_style(&draw_style);
  cig_assign_draw_rectangle(&draw_rectangle);
  cig_assign_draw_line(&draw_line);

#ifdef DEBUG
  cig_set_layout_breakpoint_callback(&layout_breakpoint);
#endif
  
  /*
   * Calling begin layout here, so that when win95 instance starts,
   * it already has a size reference to center some welcome windows into.
   */
  cig_begin_layout(&ctx, NULL, cig_r_make(0, 0, win95_w, win95_h), 0.f);

  win95_t win_instance = { 0 };
  win95_initialize(&win_instance);

  set_up_render_textures();

  bool running = true;

  while (!WindowShouldClose() && running) {
    check_alt_enter();

    BeginDrawing();

#ifdef DEBUG
    if (IsKeyPressed(KEY_F10)) {
      cig_enable_debug_stepper(true);
    }
#endif

    BeginTextureMode(render_texture);

#ifdef DEBUG
    ClearBackground((Color){0});
#endif

    cig_begin_layout(&ctx, NULL, cig_r_make(0, 0, win95_w, win95_h), GetFrameTime());
    
    cig_set_input_state(
      cig_v_make(GetMouseX()*scale, GetMouseY()*scale),
      IsMouseButtonDown(MOUSE_BUTTON_LEFT) ? CIG_INPUT_PRIMARY_ACTION : 0 +
      IsMouseButtonDown(MOUSE_BUTTON_RIGHT) ? CIG_INPUT_SECONDARY_ACTION : 0
    );
    
    running = win95_run();
    
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
CIG_INLINED void set_clip_rect(cig_buffer_ref buffer, cig_r rect, bool reset) {
  if (reset) {
    EndScissorMode();
  } else {
    BeginScissorMode(UNPACK_RECT(rect));
  }
}

static char text_api_buffer[4096];

CIG_INLINED void render_text(
  const char *str,
  size_t len,
  cig_r rect,
  cig_font_ref font,
  cig_text_color_ref color,
  cig_text_style style
) {
  strncpy(text_api_buffer, str, len);
  text_api_buffer[len] = '\0';
  
  struct font_store *fs = (struct font_store*)font;

  // printf("render: |%s|\n", buf);
  
  // DrawRectangle(UNPACK_RECT(rect), GREEN);
  DrawTextEx(fs->font, text_api_buffer, (Vector2) { rect.x, rect.y }, fs->font.baseSize, 0, *(Color*)color);
}

CIG_INLINED cig_v measure_text(
  const char *str,
  size_t len,
  cig_font_ref font,
  cig_text_style style
) {
  strncpy(text_api_buffer, str, len);
  text_api_buffer[len] = '\0';
  
  struct font_store *fs = (struct font_store*)font;
  Vector2 bounds = MeasureTextEx(fs->font, text_api_buffer, fs->font.baseSize, 0);
  
  return cig_v_make(bounds.x, bounds.y);
}

CIG_INLINED cig_font_info_st font_query(cig_font_ref font_ref) {
  struct font_store *fs = (struct font_store*)font_ref;
  
  // printf("GLYPH PADDING %d\n", fs->font.glyphPadding);
  
  return (cig_font_info_st) {
    .height = fs->font.baseSize,
    .baseline_offset = fs->baseline_offset
  };
}

CIG_INLINED void draw_style(cig_style_ref style_ref, cig_r rect, cig_style_modifiers modifiers) {
  const int style = *(int*)style_ref;
  
  switch (style) {
    case STYLE_STANDARD_DIALOG: {
      DrawRectangleRec(RAYLIB_RECT(rect), colors[COLOR_DIALOG_BACKGROUND]);

      DrawLine(rect.x, rect.y + rect.h - 1, rect.x + rect.w, rect.y + rect.h - 1, (Color) { 0, 0, 0, 255 });
      DrawLine(rect.x + rect.w, rect.y, rect.x + rect.w, rect.y + rect.h - 1, (Color) { 0, 0, 0, 255 });
      DrawLine(rect.x + 1, rect.y + rect.h - 2, rect.x + rect.w - 1, rect.y + rect.h - 2, (Color) { 130, 130, 130, 255 });
      DrawLine(rect.x + rect.w - 1, rect.y + 1, rect.x + rect.w - 1, rect.y + rect.h - 2, (Color) { 130, 130, 130, 255 });
      DrawLine(rect.x + 1, rect.y + 1, rect.x + rect.w - 2, rect.y + 1, (Color) { 255, 255, 255, 255 });
      DrawLine(rect.x + 2, rect.y + 1, rect.x + 2, rect.y + rect.h - 2, (Color) { 255, 255, 255, 255 });
    } break;
    
    case STYLE_BUTTON: {
      if (modifiers & CIG_STYLE_APPLY_SELECTION) {
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
      } else if (modifiers & CIG_STYLE_APPLY_PRESS) {
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
    
    case STYLE_LIGHT_YELLOW: {
      DrawTexturePro(images[IMAGE_BRIGHT_YELLOW_PATTERN], (Rectangle) { 0, 0, rect.w, rect.h }, (Rectangle) { rect.x, rect.y, rect.w, rect.h }, (Vector2){ 0, 0 }, 0, WHITE);
    } break;

    case STYLE_GRAY_DITHER: {
      DrawTexturePro(images[IMAGE_GRAY_DITHER], (Rectangle) { 0, 0, rect.w, rect.h }, (Rectangle) { rect.x, rect.y, rect.w, rect.h }, (Vector2){ 0, 0 }, 0, WHITE);
    } break;
    
    case STYLE_INNER_BEVEL_NO_FILL: {
      DrawLine(rect.x, rect.y, rect.x + rect.w - 1, rect.y, (Color) { 130, 130, 130, 255 });
      DrawLine(rect.x + 1, rect.y, rect.x + 1, rect.y + rect.h - 1, (Color) { 130, 130, 130, 255 });
      DrawLine(rect.x, rect.y + rect.h - 1, rect.x + rect.w , rect.y + rect.h - 1, (Color) { 255, 255, 255, 255 });
      DrawLine(rect.x + rect.w , rect.y, rect.x + rect.w , rect.y + rect.h, (Color) { 255, 255, 255, 255 });
    } break;

  case STYLE_FILES_CONTENT_BEVEL:
    {
      DrawLine(rect.x, rect.y, rect.x + rect.w - 1, rect.y, (Color) { 130, 130, 130, 255 });
      DrawLine(rect.x + 1, rect.y, rect.x + 1, rect.y + rect.h - 1, (Color) { 130, 130, 130, 255 });
      DrawLine(rect.x, rect.y + rect.h - 1, rect.x + rect.w , rect.y + rect.h - 1, (Color) { 255, 255, 255, 255 });
      DrawLine(rect.x + rect.w , rect.y, rect.x + rect.w , rect.y + rect.h, (Color) { 255, 255, 255, 255 });
      DrawLine(rect.x + 1, rect.y + 1, rect.x + rect.w - 2, rect.y + 1, (Color){ 0, 0, 0, 255 });
      DrawLine(rect.x + 2, rect.y + 2, rect.x + 2, rect.y + rect.h - 2, (Color){ 0, 0, 0, 255 });
      DrawLine(rect.x + 1, rect.y + rect.h - 2, rect.x + rect.w - 1, rect.y + rect.h - 2, colors[COLOR_DIALOG_BACKGROUND]);
      DrawLine(rect.x + rect.w - 1, rect.y + 1, rect.x + rect.w - 1, rect.y + rect.h - 2, colors[COLOR_DIALOG_BACKGROUND]);
    } break;

    case STYLE_SCROLL_BUTTON: {
      if (modifiers & CIG_STYLE_APPLY_PRESS) {
        DrawRectangle(UNPACK_RECT(rect), colors[COLOR_DIALOG_BACKGROUND]);
        DrawRectangleLinesEx(RAYLIB_RECT(rect), 1, (Color) { 128, 128, 128, 255 });
      } else {
        DrawRectangle(UNPACK_RECT(rect), colors[COLOR_DIALOG_BACKGROUND]);
        DrawLine(rect.x + 1, rect.y + 1, rect.x + rect.w - 2, rect.y + 1, (Color){ 255, 255, 255, 255 });
        DrawLine(rect.x + 2, rect.y + 2, rect.x + 2, rect.y + rect.h - 1, (Color){ 255, 255, 255, 255 });
        DrawLine(rect.x, rect.y + rect.h - 1, rect.x + rect.w, rect.y + rect.h - 1, (Color){ 0, 0, 0, 255 });
        DrawLine(rect.x + rect.w, rect.y, rect.x + rect.w, rect.y + rect.h - 1, (Color){ 0, 0, 0, 255 });
        DrawLine(rect.x + 1, rect.y + rect.h - 2, rect.x + rect.w - 1, rect.y + rect.h - 2, (Color){ 130, 130, 130, 255 });
        DrawLine(rect.x + rect.w - 1, rect.y + 1, rect.x + rect.w - 1, rect.y + rect.h - 2, (Color){ 130, 130, 130, 255 });
      }
    } break;
  }
}

CIG_INLINED void draw_rectangle(
  cig_color_ref fill_color,
  cig_color_ref border_color,
  cig_r rect,
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
  cig_v p0,
  cig_v p1,
  float thickness
) {
  DrawLineEx(RAYLIB_VEC2(p0), RAYLIB_VEC2(p1), thickness, *(Color*)color);
}

CIG_INLINED cig_v measure_image(cig_image_ref image) {
  Texture2D *tex = (Texture2D *)image;
  return cig_v_make(tex->width, tex->height);
}

CIG_INLINED void draw_image(
  cig_buffer_ref buffer,
  cig_r container,
  cig_r rect,
  cig_image_ref image,
  cig_image_mode mode
) {
  Texture2D *tex = (Texture2D *)image;

  if (dithering_shader_enabled) {
    BeginShaderMode(blue_dither_shader);
  }

  DrawTexturePro(
    *tex,
    (Rectangle) { 0, 0, tex->width, tex->height },
    RAYLIB_RECT(rect),
    (Vector2) { 0, 0 },
    0,
    WHITE
  );

  if (dithering_shader_enabled) {
    EndShaderMode();
  }
}

#ifdef DEBUG

static void layout_breakpoint(cig_r container, cig_r rect) {
  /* Ends current render texture so we could draw things as they currently stand */
  EndTextureMode();

  // TODO: Active clip rect needs to be sent here as well, and maybe visualised somehow
  EndScissorMode();

  BeginDrawing();
  BeginTextureMode(debug_texture);
  ClearBackground((Color){ 0, 0, 0, 255 });
  DrawTexturePro(
    render_texture.texture,
    (Rectangle) { 0, 0, render_texture.texture.width, -render_texture.texture.height },
    (Rectangle) { 0, 0, render_texture.texture.width, render_texture.texture.height },
    (Vector2) { 0, 0 },
    0,
    WHITE
  );
  if (container.w > 0 && container.h > 0) {
    DrawRectangleLinesEx(RAYLIB_RECT(container), 1, (Color) { 128, 0, 0, 255 });
  }
  if (rect.w > 0 && rect.h > 0) {
    DrawRectangleLinesEx(RAYLIB_RECT(rect), 1, (Color) { 255, 0, 0, 255 });
  }
  EndTextureMode();
  DrawTexturePro(
    debug_texture.texture,
    (Rectangle) { 0, 0, debug_texture.texture.width, -debug_texture.texture.height },
    (Rectangle) { 0, 0, GetScreenWidth(), GetScreenHeight() },
    (Vector2) { 0, 0 },
    0,
    WHITE
  );
  EndDrawing();

  /* Go back to previous render texture to continue with our application */
  BeginTextureMode(render_texture);

  /* You can continue automatically */
  // WaitTime(0.5);

  /* Or manually */
  while (1) {
    PollInputEvents();
    if (IsKeyPressed(KEY_SPACE) || IsKeyPressedRepeat(KEY_SPACE)) {
      break;
    } else if (IsKeyPressed(KEY_ESCAPE)) {
      cig_disable_debug_stepper();
      break;
    } else {
      WaitTime(1.0/60);
    }
  }
}

#endif
