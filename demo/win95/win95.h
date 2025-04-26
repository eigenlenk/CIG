#ifndef CIG_WIN95_DEMO_INCLUDED
#define CIG_WIN95_DEMO_INCLUDED

#include "cig.h"

typedef enum {
  FONT_REGULAR = 0,
  FONT_BOLD,
  FONT_TIMES_NEW_ROMAN_32_BOLD,
  FONT_ARIAL_BLACK_32,
  FONT_FRANKLIN_GOTHIC_BOOK_32,
  __FONT_COUNT
} font_id_t;

typedef enum {
  COLOR_BLACK = 0,
  COLOR_WHITE,
  COLOR_DESKTOP,
  COLOR_DIALOG_BACKGROUND,
  COLOR_WINDOW_ACTIVE_TITLEBAR,
  __COLOR_COUNT
} color_id_t;

typedef enum {
  IMAGE_BRIGHT_YELLOW_PATTERN = 0,
  IMAGE_START_ICON,
  __IMAGE_COUNT
} image_id_t;

typedef enum {
  PANEL_STANDARD_DIALOG = 0,
  PANEL_BUTTON,
  PANEL_LIGHT_YELLOW,
  PANEL_INNER_BEVEL_NO_FILL,
  __PANEL_COUNT
} panel_id_t;

typedef struct {
  void* (*get_font)(font_id_t);
  void* (*get_color)(color_id_t);
  void* (*get_image)(image_id_t);
  void* (*get_panel)(panel_id_t);
} win95_t;

void run_win95(win95_t *);

#endif
