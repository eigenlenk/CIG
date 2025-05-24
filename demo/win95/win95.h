#ifndef CIG_WIN95_DEMO_INCLUDED
#define CIG_WIN95_DEMO_INCLUDED

#include "cig.h"

#define WIN95_APPS_MAX 16
#define WIN95_OPEN_WINDOWS_MAX 16
#define WIN95_MAX_MENU_GROUPS 4
#define WIN95_MAX_MENU_GROUP_ITEMS 16

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
  COLOR_DEBUG,
  __COLOR_COUNT
} color_id_t;

typedef enum {
  IMAGE_BRIGHT_YELLOW_PATTERN = 0,
  IMAGE_GRAY_DITHER,
  IMAGE_START_ICON,
  IMAGE_START_SIDEBAR,
  IMAGE_MY_COMPUTER_16,
  IMAGE_MY_COMPUTER_32,
  IMAGE_TIP_OF_THE_DAY,
  IMAGE_CHECKMARK,
  IMAGE_CROSS,
  IMAGE_MAXIMIZE,
  IMAGE_MINIMIZE,
  IMAGE_RESTORE,
  IMAGE_WELCOME_APP_ICON,
  IMAGE_BIN_EMPTY,
  IMAGE_BIN_EMPTY_16,
  IMAGE_DRIVE_A_16,
  IMAGE_DRIVE_A_32,
  IMAGE_DRIVE_C_16,
  IMAGE_DRIVE_C_32,
  IMAGE_DRIVE_D_16,
  IMAGE_DRIVE_D_32,
  IMAGE_CONTROLS_FOLDER_16,
  IMAGE_CONTROLS_FOLDER_32,
  IMAGE_RESIZE_HANDLE,
  IMAGE_MENU_CHECK,
  IMAGE_MENU_CHECK_INVERTED,
  IMAGE_MENU_RADIO,
  IMAGE_MENU_RADIO_INVERTED,
  IMAGE_MENU_ARROW,
  IMAGE_MENU_ARROW_INVERTED,
  IMAGE_PROGRAM_FOLDER_24,
  IMAGE_PROGRAM_FOLDER_16,
  IMAGE_DOCUMENTS_24,
  IMAGE_SETTINGS_24,
  IMAGE_FIND_24,
  IMAGE_HELP_24,
  IMAGE_RUN_24,
  IMAGE_SHUT_DOWN_24,
  IMAGE_MAIL_16,
  IMAGE_MSDOS_16,
  IMAGE_MSN_16,
  IMAGE_EXPLORER_16,
  IMAGE_CALCULATOR_16,
  IMAGE_NOTEPAD_16,
  IMAGE_PAINT_16,
  __IMAGE_COUNT
} image_id_t;

typedef enum {
  PANEL_STANDARD_DIALOG = 0,
  PANEL_BUTTON,
  PANEL_LIGHT_YELLOW,
  PANEL_GRAY_DITHER,
  PANEL_INNER_BEVEL_NO_FILL,
  PANEL_FILES_CONTENT_BEVEL,
  __PANEL_COUNT
} panel_id_t;

struct application_t;
struct window_t;

typedef enum {
  WINDOW_CLOSE = 1,
  WINDOW_MAXIMIZE,
  WINDOW_MINIMIZE
} window_message_t;

typedef void (*win_proc_t)(struct window_t*, window_message_t*, bool);

typedef enum {
  APPLICATION_KILL = 1

} application_proc_result_t;

typedef application_proc_result_t (*application_proc_t)(struct application_t *);

typedef struct window_t {
  struct application_t *owner;
  cig_id id;
  win_proc_t proc;
  void *data;
  cig_r rect;
  cig_r rect_before_maximized;
  cig_v min_size;
  char *title;
  int icon;
  enum {
    IS_PRIMARY_WINDOW = CIG_BIT(0),
    IS_RESIZABLE = CIG_BIT(1),
    IS_MAXIMIZED = CIG_BIT(2),
    IS_MINIMIZED = CIG_BIT(3)
  } flags;
} window_t;

typedef struct application_t {
  char id[16];
  application_proc_t proc;
  void *data;
  window_t windows[1];
  enum {
    KILL_WHEN_PRIMARY_WINDOW_CLOSED = (1 << 0)
  } flags;
} application_t;

typedef struct {
  window_t windows[WIN95_OPEN_WINDOWS_MAX];
  window_t *order[WIN95_OPEN_WINDOWS_MAX];
  size_t count;
} window_manager_t;

void* get_font(font_id_t);
void* get_color(color_id_t);
void* get_image(image_id_t);
void* get_panel(panel_id_t);
/*  Applies a dark blue checkerboard dither to following image draw calls */
void enable_blue_selection_dithering(bool);

typedef struct {
  /*__PRIVATE__*/
  application_t applications[WIN95_APPS_MAX];
  window_manager_t window_manager;
  size_t running_apps;
} win95_t;

void start_win95(win95_t *);
void run_win95(win95_t *);
void win95_open_app(application_t);
application_t *win95_find_open_app(const char *);

/*  ┌────────────────┐
    │ WINDOW MANAGER │
    └────────────────┘ */

window_t* window_manager_create(window_manager_t*, application_t*, window_t);
void window_manager_close(window_manager_t*, window_t*);
void window_manager_maximize(window_manager_t *, window_t *);
void window_manager_minimize(window_manager_t *, window_t *);
void window_manager_bring_to_front(window_manager_t*, cig_id);
window_t * window_manager_find_id(window_manager_t *, cig_id);
window_t * window_manager_find_primary_window(window_manager_t*, application_t*);

/*  ┌───────────────────┐
    │ COMMON COMPONENTS │
    └───────────────────┘ */
bool standard_button(cig_r, const char *);
bool icon_button(cig_r, image_id_t);
bool checkbox(cig_r, bool *, const char *);
bool taskbar_button(cig_r, const char *, int icon, bool);

bool window_begin(window_t *, window_message_t *, bool *);
void window_end(window_t *);

bool begin_file_browser(cig_r, int, color_id_t, bool, int *);
bool file_item(image_id_t, const char*);
void end_file_browser();

typedef struct menu_item {
  cig_id id;
  image_id_t icon;
  const char *title;
  enum CIG_PACKED {
    NONE,
    TOGGLE,
    RADIO_ON,
    DISABLED,
    CHILD_MENU
  } type;
  void *data;
  void (*handler)(struct menu_item *);
} menu_item;

typedef struct menu_group {
  cig_id id;
  struct {
    size_t count;
    menu_item list[WIN95_MAX_MENU_GROUP_ITEMS];
  } items;
  void (*handler)(struct menu_group *, struct menu_item *);
} menu_group;

typedef enum {
  DROPDOWN,
  START /* Taller rows */,
  START_SUBMENU
} menu_style;

typedef struct win95_menu {
  const char *title;
  struct {
    size_t count;
    menu_group list[WIN95_MAX_MENU_GROUPS];
  } groups;
  menu_style style;
  int16_t base_width;
  void (*handler)(struct win95_menu *, struct menu_group *, struct menu_item *);
} win95_menu;

typedef struct {
  cig_v position;
  enum CIG_PACKED {
    ORIGIN_TOP_LEFT,
    ORIGIN_BOTTOM_LEFT
  } origin;
} menu_presentation;

typedef enum CIG_PACKED {
  NOT_TRACKING,
  BY_CLICK,
  BY_PRESS
} menu_tracking_st;

void menubar(size_t, win95_menu*[]);
CIG_DISCARDABLE(win95_menu *) menu_setup(
  win95_menu *,
  const char *,
  menu_style,
  void (*)(struct win95_menu *, struct menu_group *, struct menu_item *),
  size_t,
  menu_group[]
);
void menu_draw(win95_menu *, menu_presentation);
menu_tracking_st menu_track(menu_tracking_st *, win95_menu *, menu_presentation);

#endif
