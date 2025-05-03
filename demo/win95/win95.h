#ifndef CIG_WIN95_DEMO_INCLUDED
#define CIG_WIN95_DEMO_INCLUDED

#include "cig.h"

#define WIN95_APPS_MAX 16

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
  COLOR_YELLOW,
  COLOR_DESKTOP,
  COLOR_DIALOG_BACKGROUND,
  COLOR_WINDOW_ACTIVE_TITLEBAR,
  COLOR_WINDOW_INACTIVE_TITLEBAR,
  __COLOR_COUNT
} color_id_t;

typedef enum {
  IMAGE_BRIGHT_YELLOW_PATTERN = 0,
  IMAGE_GRAY_DITHER,
  IMAGE_START_ICON,
  IMAGE_MY_COMPUTER_16,
  IMAGE_MY_COMPUTER_32,
  IMAGE_TIP_OF_THE_DAY,
  IMAGE_CHECKMARK,
  IMAGE_CROSS,
  IMAGE_WELCOME_APP_ICON,
  IMAGE_BIN_EMPTY,
  __IMAGE_COUNT
} image_id_t;

typedef enum {
  PANEL_STANDARD_DIALOG = 0,
  PANEL_BUTTON,
  PANEL_LIGHT_YELLOW,
  PANEL_GRAY_DITHER,
  PANEL_INNER_BEVEL_NO_FILL,
  __PANEL_COUNT
} panel_id_t;

struct application_t;
struct window_t;

typedef enum {
  WINDOW_CLOSE = 1
} window_message_t;

typedef window_message_t (*win_proc_t)(struct window_t *);

typedef enum {
  APPLICATION_KILL = 1

} application_proc_result_t;

typedef application_proc_result_t (*application_proc_t)(struct application_t *);

typedef struct window_t {
  cig_id_t id;
  win_proc_t proc;
  void *data;
  cig_rect_t rect;
  char *title;
  int icon;
} window_t;

typedef struct application_t {
  char id[16];
  application_proc_t proc;
  void *data;
  window_t windows[1];
  bool closed;
} application_t;

void application_open_window(application_t *, window_t);

void* get_font(font_id_t);
void* get_color(color_id_t);
void* get_image(image_id_t);
void* get_panel(panel_id_t);
/*  Applies a dark blue checkerboard dither to following image draw calls */
void enable_blue_selection_dithering(bool);

typedef struct {
  /*__PRIVATE__*/
  application_t applications[WIN95_APPS_MAX];
  size_t running_apps;
} win95_t;

void start_win95(win95_t *);
void run_win95(win95_t *);
void win95_open_app(application_t);
application_t *win95_find_open_app(const char *);

/*  ┌───────────────────┐
    │ COMMON COMPONENTS │
    └───────────────────┘ */
bool standard_button(cig_rect_t, const char *);
bool checkbox(cig_rect_t, bool *, const char *);
window_message_t begin_window(window_t *);
void end_window();

#endif
