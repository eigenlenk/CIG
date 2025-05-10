#ifndef CIG_WIN95_APPS_EXPLORER_INCLUDED
#define CIG_WIN95_APPS_EXPLORER_INCLUDED

#include "../../win95.h"

extern const char *explorer_path_my_computer;
extern const char *explorer_path_recycle_bin;

application_t explorer_app();
window_t explorer_create_window(application_t*, const char*);

#endif
