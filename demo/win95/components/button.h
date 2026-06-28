#ifndef CIG_WIN95_DEMO_BUTTON_INCLUDED
#define CIG_WIN95_DEMO_BUTTON_INCLUDED

#include "cig.h"
#include "system/resources.h"

extern cig_font_ref button_font;
extern cig_text_color_ref button_title_color;
extern bool button_is_pressed;

#define STD_BTN(TITLE, KEYS...) \
  button_is_pressed = cig_focused_keys(CIG_KEYS(KEYS), CIG_KEY_PRESSED); \
  if (standard_button(_, TITLE) || cig_focused_keys(CIG_KEYS(KEYS), CIG_KEY_CLICKED))

bool
standard_button(cig_r, const char *);

bool
icon_button(cig_r, image_id_t);

bool
checkbox(cig_r, bool *, const char *);

bool
taskbar_button(cig_r, const char *, int icon, bool);

#endif
