#ifndef CIG_WIN95_DEMO_WINDOW_MANAGER_INCLUDED
#define CIG_WIN95_DEMO_WINDOW_MANAGER_INCLUDED

#include "components/window.h"

#define WIN95_OPEN_WINDOWS_MAX 16

struct application_t;

typedef struct {
  window_t windows[WIN95_OPEN_WINDOWS_MAX];
  window_t *order[WIN95_OPEN_WINDOWS_MAX];
  size_t count;
} window_manager_t;

window_t*
window_manager_create(window_manager_t*, struct application_t*, window_t);

void
window_manager_close(window_manager_t*, window_t*);

void
window_manager_maximize(window_manager_t*, window_t*);

void
window_manager_minimize(window_manager_t*, window_t*);

void
window_manager_bring_to_front(window_manager_t*, window_t*);

window_t*
window_manager_find_id(window_manager_t*, cig_id);

window_t*
window_manager_find_primary_window(window_manager_t*, struct application_t*);

#endif