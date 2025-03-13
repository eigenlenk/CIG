#include "imgui.h"
#include <allegro.h>

static int ticks = 0;

void demo_ui(BITMAP *buffer) {
  im_begin_layout(buffer, frame_make(0, 0, SCREEN_W, SCREEN_H));

  // Whole screen color
  im_fill_color(1);

  im_push_frame_insets(IM_FILL, insets_uniform(10));
  im_fill_color(3);

  /*
   * Push a vertical stack layout. If we don't specify anything about
   * the height of subframes, the height of the actual frame being
   * pushed into the frame is used. This allows stacking taller and
   * shorter elements in one go.
   */
  im_push_layout_frame_insets(IM_FILL, insets_uniform(10), &im_stack_layout_builder, (im_layout_params_t) {
    0,
    .axis = VERTICAL,
    .spacing = 4,
    .height = 64,
    .options = 0, // IM_CLIP_SUBFRAMES
  });

  im_fill_color(4);

  im_enable_scroll();
  im_current_frame()->scroll_state->offset.y += 1;

  for (int i = 0; i < 16; ++i) {
    if (im_push_frame(IM_FILL)) {
      
      /*if (i % 2) {
        im_enable_scroll();
        im_current_frame()->scroll_state->offset.x -= 1;
      }*/
      
      im_push_frame(IM_FILL);
      im_fill_color(5+i);
      im_pop_frame();

      im_pop_frame();
    }
  }

  im_pop_frame(); /* Pop stack layout */

  im_pop_frame();

  im_end_layout();

  ticks ++;
}