#include "imgui.h"
#include <allegro.h>

static int ticks = 0;

void demo_ui(BITMAP *buffer) {
	static int autoscroll_direction = 1, grid_autoscroll_direction = 1;
	
  im_begin_layout(buffer, frame_make(0, 0, SCREEN_W, SCREEN_H));

  // Whole screen color
  im_fill_color(16); // Black

  im_push_frame_insets(IM_FILL, insets_uniform(10));

  im_push_frame_builder(IM_FILL, insets_uniform(10), &im_stack_layout_builder, (im_layout_params_t) {
    0,
    .axis = HORIZONTAL,
    .spacing = 10,
    .columns = 2,
    .options = IM_DEFAULT_LAYOUT_FLAGS
  });
	
	im_stroke_color(10);
	
	/* Left content */

  im_push_frame_builder(IM_FILL, insets_uniform(10), &im_stack_layout_builder, (im_layout_params_t) {
    0,
    .axis = VERTICAL,
    .spacing = 10,
    .height = 32,
    .options = IM_DEFAULT_LAYOUT_FLAGS
  });

  im_fill_color(3);
	
  im_enable_scroll(NULL);
	
	if (autoscroll_direction == 1) { /* Scroll until the bottom of the content */
		if (im_current_element()->_scroll_state->offset.y < CIM_SCROLL_LIMIT_Y) {
			im_current_element()->_scroll_state->offset.y += 1;
		} else {
			autoscroll_direction = -1;
		}
	} else { /* Scroll back top */
		if (im_current_element()->_scroll_state->offset.y > 0) {
			im_current_element()->_scroll_state->offset.y -= 1;
		} else {
			autoscroll_direction = 1;
		}
	}
		
  for (int i = 0; i < 256; ++i) {
    if (im_push_frame(IM_FILL)) {
      
      if (i % 2) {
        im_enable_scroll(NULL);
        im_current_element()->_scroll_state->offset.x -= 1;
      }
      
      if (im_push_frame(IM_FILL)) {
				im_fill_color(i);
				im_pop_frame();
			}

      im_pop_frame();
    }
  }
	
	im_stroke_color(12);

  im_pop_frame(); /* Pop left vertical stack layout */
	

  /* Right content (a grid) */

	im_push_frame_builder(IM_FILL, insets_zero(), &im_stack_layout_builder, (im_layout_params_t) {
    0,
    .axis = HORIZONTAL | VERTICAL,
    .spacing = 5,
		.columns = 4,
		.height = 50,
    .options = IM_DEFAULT_LAYOUT_FLAGS
  });
	
	im_enable_scroll(NULL);
	
	if (grid_autoscroll_direction == 1) { /* Scroll until the bottom of the content */
		if (im_current_element()->_scroll_state->offset.y < CIM_SCROLL_LIMIT_Y) {
			im_current_element()->_scroll_state->offset.y += 1;
		} else {
			grid_autoscroll_direction = -1;
		}
	} else { /* Scroll back top */
		if (im_current_element()->_scroll_state->offset.y > 0) {
			im_current_element()->_scroll_state->offset.y -= 1;
		} else {
			grid_autoscroll_direction = 1;
		}
	}

	im_fill_color(8);
	
	for (int i = 0; i < 10; ++i) {
		if (im_push_frame(IM_FILL)) {
      im_stroke_color(15);
      im_pop_frame();
    }
	}
	
	/*
	We can do funky(TM) things as well. Midway through, we can change the grid
	to a stack by removing one of the axis, horizontal in this case. The number
	of columns no longer applies, width is set to fill but height is still 50 units.
	*/
	im_current_element()->_layout_params.axis &= ~HORIZONTAL;
	
	// im_insert_spacer(IM_FILL_CONSTANT);
	
	for (int i = 0; i < 5; ++i) {
		if (im_push_frame(IM_FILL)) {
      im_stroke_color(15);
      im_pop_frame();
    }
	}
	
  im_pop_frame(); /* Pop right grid layout */

  im_pop_frame(); /* Pop horizontal stack layout */

  im_pop_frame(); /* Pop outermost inset frame */

  im_end_layout();

  ticks ++;
}