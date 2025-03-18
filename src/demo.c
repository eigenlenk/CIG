#include "imgui.h"
#include <allegro.h>

static int ticks = 0;

void demo_ui(BITMAP *buffer) {
	static int grid_demo = 0, grid_demo_cycler;
	static int autoscroll_direction = 1, grid_autoscroll_direction = 1;
	
	if (grid_demo_cycler++ > 50) {
		grid_demo_cycler = 0;
		grid_demo = (grid_demo + 1) % 4;
	}
	
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

	if (grid_demo == 0) {
		im_push_frame_builder(IM_FILL, insets_zero(), &im_stack_layout_builder, (im_layout_params_t) {
			0,
			.axis = HORIZONTAL | VERTICAL,
			.direction = DIR_DOWN,
			.spacing = 1,
			.columns = 4,
			.height = 48,
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
		Layout params can be changed midway through, resulting in quite a flexible but
		perhaps a bit confusing way of arringing stuff. Here we remove the cell height
		meaning the next element will stretch to the bottom of the container.
		*/

		im_current_element()->_layout_params.height = 0;
		
		if (im_push_frame(IM_FILL)) {
			im_stroke_color(14);
			im_pop_frame();
		}
		
		/* And this next one will fill the whole remaining space because all constraints have been removed */
		im_current_element()->_layout_params.columns = 0;
		
		if (im_push_frame(IM_FILL)) {
			im_stroke_color(13);
			im_pop_frame();
		}
		
		im_pop_frame(); /* Pop right grid layout */
	} else if (grid_demo == 1) {
		im_push_frame_builder(IM_FILL, insets_zero(), &im_stack_layout_builder, (im_layout_params_t) {
			0,
			.axis = HORIZONTAL | VERTICAL,
			.spacing = 1,
			.columns = 4,
			.height = 48,
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
		Removing the columns limit means the next cell will calculate its width
		to fill the remaining horizontal space.
		*/

		im_current_element()->_layout_params.columns = 0;
		
		if (im_push_frame(IM_FILL)) {
			im_stroke_color(14);
			im_pop_frame();
		}

		if (im_push_frame(IM_FILL)) {
			im_stroke_color(13);
			im_pop_frame();
		}
		
		im_pop_frame(); /* Pop right grid layout */
	} else if (grid_demo == 2) {
		im_push_frame_builder(IM_FILL, insets_zero(), &im_stack_layout_builder, (im_layout_params_t) {
			0,
			.axis = HORIZONTAL | VERTICAL,
			.spacing = 1,
			.options = IM_DEFAULT_LAYOUT_FLAGS
		});
		
		/* We'll add some elements with increasing height */
		
		if (im_push_frame(frame_make(0, 0, 73, 50))) {
			im_stroke_color(15);
			im_pop_frame();
		}
		
		if (im_push_frame(frame_make(0, 0, 73, 75))) {
			im_stroke_color(15);
			im_pop_frame();
		}
		
		if (im_push_frame(frame_make(0, 0, 73, 100))) {
			im_stroke_color(15);
			im_pop_frame();
		}
		
		if (im_push_frame(frame_make(0, 0, 73, 125))) {
			im_stroke_color(15);
			im_pop_frame();
		}
		
		/* Next row will start just below the longest element (the 4th one) */
		
		if (im_push_frame(IM_FILL)) {
			im_stroke_color(14);
			im_pop_frame();
		}
		
		im_pop_frame(); /* Pop right grid layout */
	} else if (grid_demo == 3) {
		im_push_frame_builder(IM_FILL, insets_zero(), &im_stack_layout_builder, (im_layout_params_t) {
			0,
			.axis = HORIZONTAL | VERTICAL,
			.direction = DIR_DOWN,
			.spacing = 5,
			.options = IM_DEFAULT_LAYOUT_FLAGS
		});
		
		/* We'll add some elements with increasing height */
		
		if (im_push_frame(frame_make(0, 0, 50, 84))) {
			im_stroke_color(15);
			im_pop_frame();
		}
		
		if (im_push_frame(frame_make(0, 0, 60, 84))) {
			im_stroke_color(15);
			im_pop_frame();
		}
		
		if (im_push_frame(frame_make(0, 0, 70, 84))) {
			im_stroke_color(15);
			im_pop_frame();
		}
		
		if (im_push_frame(frame_make(0, 0, 80, 84))) {
			im_stroke_color(15);
			im_pop_frame();
		}
		
		if (im_push_frame(frame_make(0, 0, 90, 84))) {
			im_stroke_color(15);
			im_pop_frame();
		}
		
		/* Next columns will start just after the widest element (the 5th one) */
		
		if (im_push_frame(IM_FILL)) {
			im_stroke_color(14);
			im_pop_frame();
		}
		
		im_pop_frame(); /* Pop right grid layout */
	}

  im_pop_frame(); /* Pop horizontal stack layout */

  im_pop_frame(); /* Pop outermost inset frame */

  im_end_layout();

  ticks ++;
}