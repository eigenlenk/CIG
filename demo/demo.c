#include "imgui.h"
#include <allegro.h>

static int ticks = 0;

void demo_ui(BITMAP *buffer) {
	static int grid_demo = 0;
	static int autoscroll_direction = 1;
	
	/*if (grid_demo_cycler++ > 80) {
		grid_demo_cycler = 0;
		grid_demo = (grid_demo + 1) % 4;
	}*/
	
  cig_begin_layout(buffer, cig_frame_make(0, 0, SCREEN_W, SCREEN_H));
	
	/* Pass cursor position and mouse button states */
	im_set_input_state(
		vec2_make(mouse_x, mouse_y),
		(mouse_b & 1 ? IM_MOUSE_BUTTON_LEFT : 0) +
		(mouse_b & 2 ? IM_MOUSE_BUTTON_RIGHT : 0)
	);

  // Whole screen color
  CIG_FILL_color(16); // Black

  cig_push_frame_insets(CIG_FILL, cig_insets_uniform(10));

  im_push_frame_builder(CIG_FILL, cig_insets_uniform(10), &im_stack_layout_builder, (im_layout_params_t) {
    0,
    .axis = HORIZONTAL,
    .spacing = 10,
    .columns = 2,
    .options = IM_DEFAULT_LAYOUT_FLAGS
  });
	
	im_stroke_color(10);
	
	/* Left content */

  im_push_frame_builder(CIG_FILL, cig_insets_uniform(10), &im_stack_layout_builder, (im_layout_params_t) {
    0,
    .axis = VERTICAL,
    .spacing = 10,
    .height = 32,
    .options = IM_DEFAULT_LAYOUT_FLAGS
  });

  CIG_FILL_color(3);
	
  im_enable_scroll(NULL);
	im_enable_interaction();
	
	if (!im_hovered()) {
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
	}
		
  for (int i = 0; i < 24; ++i) {
    if (cig_push_frame(CIG_FILL)) {
      
      if (i % 2) {
        im_enable_scroll(NULL);
        im_current_element()->_scroll_state->offset.x -= 1;
      }
      
      if (cig_push_frame(CIG_FILL)) {
				CIG_FILL_color(i);
				cig_pop_frame();
			}

      cig_pop_frame();
    }
  }
	
	im_stroke_color(12);

  cig_pop_frame(); /* Pop left vertical stack layout */
	

  /* Right content (a grid) */
	
	void demo_grid_element(const cig_frame_t frame) {
		if (cig_push_frame(frame)) {
			im_enable_interaction();
			if (im_pressed(IM_MOUSE_BUTTON_ANY, 0)) {
				CIG_FILL_color(75);
			} else if (im_hovered()) {
				CIG_FILL_color(7);
			}
			im_stroke_color(15);
			cig_pop_frame();
		}
	}

	if (grid_demo == 0) {
		im_push_frame_builder(CIG_FILL, cig_insets_zero(), &im_stack_layout_builder, (im_layout_params_t) {
			0,
			.axis = HORIZONTAL | VERTICAL,
			.direction = DIR_DOWN,
			.spacing = 1,
			.columns = 4,
			.height = 48,
			.options = IM_DEFAULT_LAYOUT_FLAGS
		});
		
		CIG_FILL_color(8);
		
		for (int i = 0; i < 10; ++i) {
			demo_grid_element(CIG_FILL);
		}
		
		/*
		Layout params can be changed midway through, resulting in quite a flexible but
		perhaps a bit confusing way of arringing stuff. Here we remove the cell height
		meaning the next element will stretch to the bottom of the container.
		*/
		im_current_element()->_layout_params.height = 0;
		
		demo_grid_element(CIG_FILL);
		
		/* And this next one will fill the whole remaining space because all constraints have been removed */
		im_current_element()->_layout_params.columns = 0;
		
		demo_grid_element(CIG_FILL);
		
		cig_pop_frame(); /* Pop right grid layout */
	} else if (grid_demo == 1) {
		im_push_frame_builder(CIG_FILL, cig_insets_zero(), &im_stack_layout_builder, (im_layout_params_t) {
			0,
			.axis = HORIZONTAL | VERTICAL,
			.spacing = 1,
			.columns = 4,
			.height = 48,
			.options = IM_DEFAULT_LAYOUT_FLAGS
		});
		
		CIG_FILL_color(8);
		
		for (int i = 0; i < 10; ++i) {
			demo_grid_element(CIG_FILL);
		}
		
		/*
		Removing the columns limit means the next cell will calculate its width
		to fill the remaining horizontal space.
		*/
		im_current_element()->_layout_params.columns = 0;
		
		demo_grid_element(CIG_FILL);
		demo_grid_element(CIG_FILL);
		
		cig_pop_frame(); /* Pop right grid layout */
	} else if (grid_demo == 2) {
		im_push_frame_builder(CIG_FILL, cig_insets_zero(), &im_stack_layout_builder, (im_layout_params_t) {
			0,
			.axis = HORIZONTAL | VERTICAL,
			.spacing = 1,
			.options = IM_DEFAULT_LAYOUT_FLAGS
		});
		
		CIG_FILL_color(8);
		
		/* We'll add some elements with increasing height */
		demo_grid_element(cig_frame_make(0, 0, 73, 50));
		demo_grid_element(cig_frame_make(0, 0, 73, 75));
		demo_grid_element(cig_frame_make(0, 0, 73, 100));
		demo_grid_element(cig_frame_make(0, 0, 73, 125));
		
		/* Next row will start just below the longest element (the 4th one) */
		demo_grid_element(CIG_FILL);

		cig_pop_frame(); /* Pop right grid layout */
	} else if (grid_demo == 3) {
		im_push_frame_builder(CIG_FILL, cig_insets_zero(), &im_stack_layout_builder, (im_layout_params_t) {
			0,
			.axis = HORIZONTAL | VERTICAL,
			.direction = DIR_DOWN,
			.spacing = 5,
			.options = IM_DEFAULT_LAYOUT_FLAGS
		});
		
		CIG_FILL_color(8);
		
		/* We'll add some elements with increasing height */
		demo_grid_element(cig_frame_make(0, 0, 50, 84));
		demo_grid_element(cig_frame_make(0, 0, 60, 84));
		demo_grid_element(cig_frame_make(0, 0, 70, 84));
		demo_grid_element(cig_frame_make(0, 0, 80, 84));
		demo_grid_element(cig_frame_make(0, 0, 90, 84));
		
		/* Next columns will start just after the widest element (the 5th one) */
		demo_grid_element(CIG_FILL);
		
		cig_pop_frame(); /* Pop right grid layout */
	}

  cig_pop_frame(); /* Pop horizontal stack layout */
	
	/* Some sort of a floating footer button on top of the content */
	{
		cig_push_frame(cig_frame_make(200, IM_B - 100, 240, 50));
		
		im_enable_interaction();
		
		if (im_clicked(IM_MOUSE_BUTTON_ANY, IM_CLICK_STARTS_INSIDE)) {
			grid_demo = (grid_demo + 1) % 4;
			CIG_FILL_color(3);
		} else if (im_pressed(IM_MOUSE_BUTTON_ANY, IM_MOUSE_PRESS_INSIDE)) {
			CIG_FILL_color(9);
		} else if (im_hovered()) {
			CIG_FILL_color(10);
		}
		
		im_stroke_color(15);
		
		cig_pop_frame();
	}

  cig_pop_frame(); /* Pop outermost inset frame */

  cig_end_layout();

  ticks ++;
}