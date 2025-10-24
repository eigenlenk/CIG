#include "notepad.h"
#include "components/scroller.h"

static void
window_proc(window_t *this, bool window_focused)
{
  cig_frame *window_content_area = cig_current();

  CIG(
    BUILD_RECT(
      PIN(LEFT_OF(window_content_area)),
      PIN(RIGHT_OF(window_content_area)),
      PIN(TOP_OF(window_content_area), OFFSET_BY(18)),
      PIN(BOTTOM_OF(window_content_area))
    ),
    CIG_INSETS(cig_i_uniform(2))
  ) {
    cig_fill_color(get_color(COLOR_WHITE));
    cig_fill_style(get_style(STYLE_FILES_CONTENT_BEVEL), 0);

    CIG(_) {
      cig_enable_scroll(NULL);

      display_scrollbars(cig_scroll_state(), SCROLLER_ALWAYS_VISIBLE, NULL);
    }
  }

  if (!(this->flags & IS_MAXIMIZED)) {
    CIG(RECT(CIG_W_INSET-14, CIG_H_INSET-14, 12, 12)) {
      cig_draw_image(get_image(IMAGE_RESIZE_HANDLE), CIG_IMAGE_MODE_TOP_LEFT);
    }
  }
}

application_t
notepad_app()
{
  return (application_t) {
    .id = "notepad",
    .windows = {
      (window_t) {
        .id = cig_hash("notepad"),
        .proc = &window_proc,
        .data = NULL,
        .rect = CENTER_APP_WINDOW(300, 340),
        .min_size = { 220, 160 },
        .title = "Notepad",
        .icon = IMAGE_NOTEPAD_16,
        .flags = IS_PRIMARY_WINDOW | IS_RESIZABLE
      }
    },
    // .data = state,
    .flags = KILL_WHEN_PRIMARY_WINDOW_CLOSED
  };
}
