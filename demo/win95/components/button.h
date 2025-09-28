#ifndef CIG_WIN95_DEMO_BUTTON_INCLUDED
#define CIG_WIN95_DEMO_BUTTON_INCLUDED

#include "cig.h"
#include "system/resources.h"

extern cig_font_ref button_font;

bool
standard_button(cig_r, const char *);

bool
icon_button(cig_r, image_id_t);

bool
checkbox(cig_r, bool *, const char *);

bool
taskbar_button(cig_r, const char *, int icon, bool);

#endif