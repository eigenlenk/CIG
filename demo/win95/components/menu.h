#ifndef CIG_WIN95_DEMO_MENU_INCLUDED
#define CIG_WIN95_DEMO_MENU_INCLUDED

#include "cig.h"
#include "system/resources.h"

#define WIN95_MAX_MENU_GROUPS 4
#define WIN95_MAX_MENU_GROUP_ITEMS 16

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
  void (*handler)(struct menu_item*);
  void (*_handler)();
} menu_item;

typedef struct menu_group {
  cig_id id;
  struct {
    size_t count;
    menu_item list[WIN95_MAX_MENU_GROUP_ITEMS];
  } items;
  void (*handler)(struct menu_group*, struct menu_item*);
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
  void (*handler)(struct win95_menu*, struct menu_group*, struct menu_item*);
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

/* API */

void
menubar(size_t, win95_menu*[]);

CIG_DISCARDABLE(win95_menu *)
menu_setup(
  win95_menu*,
  const char*,
  menu_style,
  void (*)(struct win95_menu*, struct menu_group*, struct menu_item*),
  size_t,
  menu_group[]
);

void
menu_draw(win95_menu *, menu_presentation);

menu_tracking_st
menu_track(menu_tracking_st*, win95_menu*, menu_presentation);

#endif
