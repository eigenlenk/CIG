#include "win95.h"
#include "cigcorem.h"

#include <string.h>
#include <stdint.h>
#include <stdio.h>

void menubar(size_t n, win95_menu* menus[]) {
  register size_t i;

  struct menubar_st {
    enum { NONE, BY_CLICK, BY_PRESS } tracking;
    uint16_t active_menu_idx;
  };

  struct menubar_st *this = CIG_ALLOCATE(struct menubar_st);
  cig_v menu_position;

  cig_retain(cig_current());

  /* Menubar just became visible (parent window appeared basically) */
  if (cig_visibility() == CIG_FRAME_APPEARED) {
    this->tracking = NONE;
  }

  /* */
  cig_enable_interaction();
  cig_disable_culling();

  CIG_HSTACK(RECT_AUTO, NO_INSETS) {
    for (i = 0; i < n; ++i) {
      /*
       * Calculate title label size. Using raw text API here because of how
       * relatively short these labels are. Raw text is drawn using screen position
       * and we must center it vertically ourselves.
       */
      cig_v text_size = cig_measure_raw_text(NULL, 0, menus[i]->title);

      CIG(RECT_AUTO_W(text_size.x + 6*2), cig_i_horizontal(6)) {
        cig_enable_interaction();

        /* Click or press any menubar item to activate tracking */
        if (this->tracking == NONE && cig_pressed(CIG_INPUT_PRIMARY_ACTION, CIG_PRESS_INSIDE)) {
          this->tracking = BY_PRESS;
          printf("Start tracking [by press]\n");
        } else if (cig_clicked(CIG_INPUT_PRIMARY_ACTION, CIG_CLICK_STARTS_INSIDE | CIG_CLICK_EXPIRE)) {
          this->tracking = this->tracking != BY_CLICK ? BY_CLICK : NONE;
          if (this->tracking) {
            printf("Start tracking [by click]\n");
          } else {
            printf("Stop tracking [by click]\n");
          }
        }

        /* Change active menu to current one */
        if (this->tracking && cig_hovered()) {
          this->active_menu_idx = i;
        }
        
        /* Draw menubar item based on tracking state */
        const bool menu_open = this->tracking && this->active_menu_idx == i;
        
        if (menu_open) {
          menu_position = cig_v_make(CIG_X, CIG_B);
          cig_fill_solid(get_color(COLOR_WINDOW_ACTIVE_TITLEBAR));
        }

        cig_draw_raw_text(
          cig_v_make(CIG_SX_INSET, CIG_SY_INSET + (CIG_H - text_size.y) * 0.5),
          text_size,
          NULL,         /* Font */
          0,            /* Text style modifiers */
          menu_open ? get_color(COLOR_WHITE) : get_color(COLOR_BLACK),
          menus[i]->title
        );
      }
    }

    /* Fill rest of the menubar */
    CIG(_) {
      cig_enable_interaction();
      if (this->tracking && cig_pressed(CIG_INPUT_PRIMARY_ACTION, CIG_PRESS_INSIDE)) {
        this->tracking = NONE;
        printf("Stop tracking [outside menu items]\n");
      }
    }
  }

  /* Draw open menu */
  if (this->tracking) {
    menu_draw(menus[this->active_menu_idx], (menu_presentation) {
      .position = menu_position
    });
  }

  if (this->tracking == BY_CLICK && cig_input_state()->click_state == EXPIRED) {
    this->tracking = NONE;
    printf("Stop tracking [click expired]\n");
  } else if (this->tracking == BY_PRESS && cig_input_state()->action_mask == 0) {
    /* Mouse button was released while in PRESS tracking mode */
    this->tracking = NONE;
    printf("Stop tracking [button released]\n");
  } else if (this->tracking == BY_CLICK && cig_input_state()->click_state == BEGAN && !(cig_current()->_flags & SUBTREE_INCLUSIVE_HOVER)) {
    /* Mouse button was pressed down while outside the menubar and any of its children (menus) */
    this->tracking = NONE;
    printf("Stop tracking [outside menubar]\n");
  } else if (this->tracking == BY_CLICK && cig_input_state()->click_state == ENDED && !(cig_current()->_flags & HOVER) && cig_current()->_flags & SUBTREE_INCLUSIVE_HOVER) {
    /* Click was made, but inside one of its children (menus) and not the bar itself */
    this->tracking = NONE;
    printf("Stop tracking [inside menu]\n");
  }
}

win95_menu * menu_setup(
  win95_menu *this,
  const char *title,
  menu_style style,
  void (*handler)(struct win95_menu *, struct menu_group *, struct menu_item *),
  size_t n,
  menu_group groups[]
) {
  register int i, j, w;
  char *item_title;

  this->title = title;
  this->style = style;
  this->groups.count = n;
  this->base_width = 0;
  this->handler = handler;
  memcpy(this->groups.list, groups, n * sizeof(menu_group));

  for (i = 0; i < n; ++i) {
    /* Find widest text element */
    for (j = 0; j < groups[i].items.count; ++j) {
      if (groups[i].items.list[j].type == CHILD_MENU) {
        item_title = (char *)((win95_menu *)groups[i].items.list[j].data)->title;
      } else {
        item_title = (char *)groups[i].items.list[j].title;
      }
      w = cig_measure_raw_text(NULL, 0, item_title).x;
      if (w > this->base_width) {
        this->base_width = w;
      }
    }
  }

  return this;
}

void menu_draw(win95_menu *this, menu_presentation presentation) {
  register int i, j;
  menu_group *group;
  menu_item *item;
  win95_menu *child_menu;
  char *item_title;
  struct {
    cig_i insets;
    int16_t height,
            icon_width,
            icon_title_spacing,
            after_title_spacing;
  } size_info;

  const cig_i panel_insets = cig_i_uniform(3);

  switch (this->style) {
  case DROPDOWN:
    size_info.insets = cig_i_horizontal(6);
    size_info.height = 18;
    size_info.icon_width = 8;
    size_info.icon_title_spacing = 6;
    size_info.after_title_spacing = 15;
    break;
  case START:
    break;
  case START_SUBMENU:
    break;
  }

  int panel_width = panel_insets.left
    + size_info.insets.left
    + size_info.icon_width +
    + size_info.icon_title_spacing
    + this->base_width
    + size_info.after_title_spacing
    + size_info.insets.right
    + panel_insets.right;

  int panel_height = panel_insets.top + panel_insets.bottom;

  for (i = 0; i < this->groups.count; ++i) {
    panel_height += this->groups.list[i].items.count * size_info.height;
    if (i < this->groups.count - 1) {
      panel_height += 8; /* Separator */
    }
  }

  cig_set_next_id(cig_hash(this->title));
  CIG(
    RECT(presentation.position.x, presentation.position.y, panel_width, panel_height),
    CIG_INSETS(panel_insets)
  ) {
    cig_retain(cig_current());

    win95_menu **presented_submenu = CIG_ALLOCATE(win95_menu*);
    cig_v *submenu_position = CIG_ALLOCATE(cig_v);

    cig_fill_panel(get_panel(PANEL_STANDARD_DIALOG), 0);

    cig_enable_interaction();

    if (cig_visibility() == CIG_FRAME_APPEARED || cig_hovered()) {
      /* Hovering non-content (edges, separators) */
      *presented_submenu = NULL;
    }
    
    CIG_VSTACK(_, NO_INSETS, CIG_PARAMS({
      CIG_HEIGHT(18)
    })) {
      for (i = 0; i < this->groups.count; ++i) {
        group = &this->groups.list[i];

        for (j = 0; j < group->items.count; ++j) {
          item = &group->items.list[j];

          CIG(_) {
            cig_enable_interaction();

            const bool item_hovered = cig_hovered();
            bool draws_highlight = item_hovered && item->type != DISABLED;

            if (item->type == CHILD_MENU && (child_menu = (win95_menu *)item->data)) {
              item_title = (char *)child_menu->title;
            } else {
              item_title = (char *)item->title;
            }

            if (item_hovered) {
              if (item->type == CHILD_MENU) {
                *presented_submenu = child_menu;
                *submenu_position = cig_v_make(CIG_W_INSET - 3, CIG_Y - 3);
              } else {
                *presented_submenu = NULL;
              }
            } else {
              if (item->type == CHILD_MENU) {
                draws_highlight = *presented_submenu == child_menu;
              }
            }

            if (draws_highlight) {
              cig_fill_solid(get_color(COLOR_WINDOW_ACTIVE_TITLEBAR));
            }

            if (item->type != DISABLED && item->type != CHILD_MENU) {
              if (cig_clicked(CIG_INPUT_PRIMARY_ACTION, 0)) {
                if (item->type == TOGGLE) {
                  bool *bool_value = (bool *)item->data;
                  if (bool_value) { *bool_value = !*bool_value; }
                }
                if (item->handler) {
                  item->handler(item);
                } else if (group->handler) {
                  group->handler(group, item);
                } else if (this->handler) {
                  this->handler(this, group, item);
                } else {
                  printf("Note: No menu handler configured for '%s'\n", this->title);
                }
              }
            }

            CIG_HSTACK(_, size_info.insets) {
              CIG(_W(size_info.icon_width)) {
                if (item->type == TOGGLE) {
                  bool *bool_value = (bool *)item->data;
                  if (bool_value && *bool_value) {
                    cig_draw_image(get_image(item_hovered ? IMAGE_MENU_CHECK_INVERTED : IMAGE_MENU_CHECK), CIG_IMAGE_MODE_CENTER);
                  }
                } else if (item->type == RADIO_ON) {
                  cig_draw_image(get_image(item_hovered ? IMAGE_MENU_RADIO_INVERTED : IMAGE_MENU_RADIO), CIG_IMAGE_MODE_CENTER);
                }
              }

              CIG(_W(size_info.icon_title_spacing)) {} /* Spacer */

              CIG(_W(this->base_width)) {
                cig_draw_label((cig_text_properties) {
                  .color = item->type == DISABLED
                    ? get_color(COLOR_WINDOW_INACTIVE_TITLEBAR)
                    : draws_highlight
                      ? get_color(COLOR_WHITE)
                      : get_color(COLOR_BLACK),
                  .alignment.horizontal = CIG_TEXT_ALIGN_LEFT,
                }, item_title);
              }

              if (item->type == CHILD_MENU) {
                CIG(_W(size_info.after_title_spacing)) {
                  cig_draw_image(get_image(draws_highlight ? IMAGE_MENU_ARROW_INVERTED : IMAGE_MENU_ARROW), CIG_IMAGE_MODE_RIGHT);
                }
              }
            }
          }
        }

        if (i < this->groups.count - 1) {
          CIG(_H(8)) {
            CIG(RECT_CENTERED_VERTICALLY(_H(2))) { /* Separator */
              cig_fill_panel(get_panel(PANEL_INNER_BEVEL_NO_FILL), 0);
            }
          }
        }
      }
    }

    if (*presented_submenu) {
      menu_draw(*presented_submenu, (menu_presentation) {
        .position = *submenu_position
      });
    }
  }
}