#ifndef CIG_WIN95_DEMO_WINDOW_INCLUDED
#define CIG_WIN95_DEMO_WINDOW_INCLUDED

#include "cig.h"

struct window_t;
struct window_manager_t;
struct application_t;

typedef enum {
  WINDOW_CLOSE = 1,
  WINDOW_MAXIMIZE,
  WINDOW_MINIMIZE
} window_message_t;

typedef void (*win_proc_t)(struct window_t*, window_message_t*, bool);

typedef struct window_t {
  struct window_manager_t *manager;
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
    IS_UNIQUE_WINDOW = CIG_BIT(1), /* One instance of this window ID per app */
    IS_RESIZABLE = CIG_BIT(2),
    IS_MAXIMIZED = CIG_BIT(3),
    IS_MINIMIZED = CIG_BIT(4)
  } flags;
} window_t;

bool
window_begin(window_t*, window_message_t*, bool*);

void
window_end(window_t*);

#endif