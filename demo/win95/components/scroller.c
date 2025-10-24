#include "components/scroller.h"
#include "components/button.h"
#include "system/resources.h"

#include <stdio.h>

#define SCROLL_BAR_SIZE 16
#define SCROLL_BAR_MINIMUM_THUMB 8

static bool
scroll_bar_button(cig_r, image_id_t);

void
scroll_bar(cig_r rect, int32_t *value, int32_t limit)
{
  static struct {
    int32_t *value;
    int32_t initial_value;
    bool active;
  } thumb_state = { 0 };

  cig_push_frame(rect);

  const bool is_y_axis = rect.h > rect.w;
  const int32_t visible = is_y_axis ? rect.h : rect.w;
  const int32_t total = visible + limit;
  const int32_t track = visible - 2 * SCROLL_BAR_SIZE;
  const bool is_scrollable = limit > 0;
  const int32_t thumb_size = is_scrollable
    ? CIG_MAX((visible * track) / total, SCROLL_BAR_MINIMUM_THUMB)
    : 0;
  const int32_t thumb_offset = is_scrollable
    ? (*value / (double)limit) * (track - thumb_size)
    : 0;
  const int32_t scroll_step = is_scrollable
    ? limit / (track - thumb_size)
    : 0;

  const cig_r negative_button_frame = is_y_axis
    ? BUILD_RECT(PIN(TOP), PIN(LEFT), PIN(RIGHT), PIN(ASPECT, 1))
    : BUILD_RECT(PIN(LEFT), PIN(TOP), PIN(BOTTOM), PIN(ASPECT, 1));

  const cig_r positive_button_frame = is_y_axis
    ? BUILD_RECT(PIN(BOTTOM), PIN(LEFT), PIN(RIGHT), PIN(ASPECT, 1))
    : BUILD_RECT(PIN(RIGHT), PIN(TOP), PIN(BOTTOM), PIN(ASPECT, 1));

  cig_frame *scroll_negative, *scroll_positive;

  CIG_RETAIN(
    scroll_negative,
    if (scroll_bar_button(negative_button_frame, is_y_axis ? IMAGE_SCROLL_UP : IMAGE_SCROLL_LEFT)) {
      *value -= scroll_step * 5;
    }
  )

  CIG_RETAIN(
    scroll_positive,
    if (scroll_bar_button(positive_button_frame, is_y_axis ? IMAGE_SCROLL_DOWN : IMAGE_SCROLL_RIGHT)) {
      *value += scroll_step * 5;
    }
  )

  const cig_r track_frame = is_y_axis
    ? BUILD_RECT(PIN(BELOW(scroll_negative)), PIN(LEFT), PIN(RIGHT), PIN(ABOVE(scroll_positive)))
    : BUILD_RECT(PIN(AFTER(scroll_negative)), PIN(TOP), PIN(BOTTOM), PIN(BEFORE(scroll_positive)));

  cig_r thumb_frame = is_y_axis
    ? BUILD_RECT(PIN(BELOW(scroll_negative), OFFSET_BY(thumb_offset)), PIN(LEFT), PIN(RIGHT), PIN(HEIGHT, thumb_size))
    : BUILD_RECT(PIN(AFTER(scroll_negative), OFFSET_BY(thumb_offset)), PIN(TOP), PIN(BOTTOM), PIN(WIDTH, thumb_size));

  CIG(track_frame) {
    cig_fill_style(get_style(STYLE_GRAY_DITHER), 0);
  }

  if (thumb_size) {
    CIG(thumb_frame) {
      cig_enable_interaction();

      switch (cig_dragged(CIG_INPUT_PRIMARY_ACTION)) {
      case CIG_DRAG_STATE_BEGAN:
        cig_input_state()->locked = true;
        thumb_state.initial_value = *value;
        thumb_state.value = value;
        /* Fallthrough */

      case CIG_DRAG_STATE_MOVED:
        *value = thumb_state.initial_value +
          (is_y_axis ? cig_input_state()->drag.change_total.y : cig_input_state()->drag.change_total.x) * scroll_step;
        break;

      default: break;
      }

      cig_fill_style(get_style(STYLE_SCROLL_BUTTON), 0);
    }
  }

  *value = CIG_CLAMP(*value, 0, limit);

  cig_pop_frame();
}

void
display_scrollbars(cig_scroll_state_t *scroll, scroller_flags flags, scroller_results *results)
{
  const bool scrolls_x = scroll->distance.x > 0;
  const bool scrolls_y = scroll->distance.y > 0;

  int32_t extra_margin_x = 0, extra_margin_y = 0;

  /*
   * Need 2 passes to get the extra padding right. Adding a scrollbar on one axis may require
   * additional space on the second axis also requiring a scrollbar, which in turn increases
   * the scrollable area on the first axis.
   */ 
  if (scrolls_y)
    extra_margin_x = CIG_CLAMP((scroll->bounds.x + SCROLL_BAR_SIZE - cig_rect().w), 0, SCROLL_BAR_SIZE);

  if (scrolls_x)
    extra_margin_y = CIG_CLAMP((scroll->bounds.y + SCROLL_BAR_SIZE - cig_rect().h), 0, SCROLL_BAR_SIZE);

  if (extra_margin_y)
    extra_margin_x = CIG_CLAMP((scroll->bounds.x + SCROLL_BAR_SIZE - cig_rect().w), 0, SCROLL_BAR_SIZE);

  if (extra_margin_x)
    extra_margin_y = CIG_CLAMP((scroll->bounds.y + SCROLL_BAR_SIZE - cig_rect().h), 0, SCROLL_BAR_SIZE);

  const int32_t total_dist_x = scroll->distance.x + extra_margin_x;
  const int32_t total_dist_y = scroll->distance.y + extra_margin_y;
  const bool shows_x_axis = (total_dist_x > 0) || (flags & SCROLLER_ALWAYS_VISIBLE_X);
  const bool shows_y_axis = (total_dist_y > 0) || (flags & SCROLLER_ALWAYS_VISIBLE_Y);
  const bool shows_both_axis = shows_x_axis && shows_y_axis;

  if (shows_x_axis) {
    scroll_bar(
      BUILD_RECT(
        PIN(LEFT_OF(cig_current())),
        PIN(BOTTOM_OF(cig_current())),
        PIN(RIGHT_OF(cig_current()), OFFSET_BY(shows_both_axis ? SCROLL_BAR_SIZE : 0)),
        PIN(HEIGHT, SCROLL_BAR_SIZE)
      ),
      &scroll->offset.x,
      total_dist_x
    );

    if (results) {
      *results |= SCROLLER_DISPLAYED_X;
    }
  } else {
    scroll->offset.x = 0;
  }

  if (shows_y_axis) {
    scroll_bar(
      BUILD_RECT(
        PIN(TOP_OF(cig_current())),
        PIN(BOTTOM_OF(cig_current()), OFFSET_BY(shows_both_axis ? SCROLL_BAR_SIZE : 0)),
        PIN(RIGHT_OF(cig_current())),
        PIN(WIDTH, SCROLL_BAR_SIZE)
      ),
      &scroll->offset.y,
      total_dist_y
    );

    if (results) {
      *results |= SCROLLER_DISPLAYED_Y;
    }
  } else {
    scroll->offset.y = 0;
  }

  if (shows_both_axis) {
    CIG(BUILD_RECT(PIN(BOTTOM), PIN(RIGHT), PIN(WIDTH, SCROLL_BAR_SIZE), PIN(HEIGHT, SCROLL_BAR_SIZE))) {
      cig_fill_color(get_color(COLOR_DIALOG_BACKGROUND));
    }
  }
}

static bool
scroll_bar_button(cig_r rect, image_id_t image_id)
{
  bool clicked = false;
  
  CIG(rect) {
    cig_enable_interaction();
    
    const bool pressed = cig_pressed(CIG_INPUT_PRIMARY_ACTION, CIG_PRESS_INSIDE);
    
    cig_fill_style(get_style(STYLE_SCROLL_BUTTON), pressed ? CIG_STYLE_APPLY_PRESS : 0);
    
    if (cig_push_frame_insets(RECT_AUTO, pressed ? cig_i_make(3, 3, 1, 1) : cig_i_make(2, 2, 2, 2))) {
      cig_draw_image(get_image(image_id), CIG_IMAGE_MODE_CENTER);
      cig_pop_frame();
    }
    
    clicked = cig_clicked(CIG_INPUT_PRIMARY_ACTION, CIG_CLICK_DEFAULT_OPTIONS);
  }
  
  return clicked;
}
