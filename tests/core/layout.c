#include "unity.h"
#include "fixture.h"
#include "imgui.h"
#include "asserts.h"

TEST_GROUP(core_layout);

TEST_SETUP(core_layout) {
	/* Begin laying out a screen that's 640 by 480 */
	im_begin_layout(NULL, frame_make(0, 0, 640, 480));
}

TEST_TEAR_DOWN(core_layout) {
	im_end_layout();
}

/* (( TEST CASES )) */

TEST(core_layout, basic_check) {
	/* Nothing really to test here, just checking nothing crashes =) */
	TEST_ASSERT_EQUAL_FRAME(frame_make(0, 0, 640, 480), im_get_element()->frame);
}

TEST(core_layout, push_pop) {
	TEST_ASSERT_EQUAL(im_get_depth(), 1); /* Just the root */
	
	im_push_frame(IM_FILL);
		im_push_frame(IM_FILL);
			im_push_frame(IM_FILL);
			
				TEST_ASSERT_EQUAL(im_get_depth(), 4);
				
			im_pop_frame();
		im_pop_frame();
	
		TEST_ASSERT_EQUAL(im_get_depth(), 2);

		im_push_frame(IM_FILL);
		
			TEST_ASSERT_EQUAL(im_get_depth(), 3);
			
		im_pop_frame();
	im_pop_frame();
	
	TEST_ASSERT_EQUAL(im_get_depth(), 1); /* Just the root again */
}

TEST(core_layout, identifiers) {
	register int a, b, c, d;
	struct {
		int n;
		IMGUIID recorded[2048];
	} ids = { 0 };
	
	bool assert_unique(const IMGUIID id, int line) {
		register int i;
		for (i = 0; i < ids.n; ++i) {
			if (ids.recorded[i] == id) {
				TEST_PRINTF("Line %d: ID %lu is not unique! [%d] = %lu", line, id, i, ids.recorded[i]);
				TEST_FAIL();
				break;
			}
		}
	}
	
	TEST_ASSERT_EQUAL_UINT32(4151533312l, im_get_element()->id);
	
	const int n0 = 8;
	
	for (a = 0; a < n0; ++a) {
		if (im_push_frame(IM_FILL)) {
			assert_unique(im_get_element()->id, __LINE__);
			ids.recorded[ids.n++] = im_get_element()->id;
			const int n1 = 8;
			for (b = 0; b < n1; ++b) {
				if (im_push_frame(IM_FILL)) {
					assert_unique(im_get_element()->id, __LINE__);
					ids.recorded[ids.n++] = im_get_element()->id;
					const int n2 = 8;
					for (c = 0; c < n2; ++c) {
						if (im_push_frame(IM_FILL)) {
							assert_unique(im_get_element()->id, __LINE__);
							ids.recorded[ids.n++] = im_get_element()->id;
							const int n3 = 8;
							for (d = 0; d < n3; ++d) {
								if (im_push_frame(IM_FILL)) {
									assert_unique(im_get_element()->id, __LINE__);
									ids.recorded[ids.n++] = im_get_element()->id;
									im_pop_frame();
								}
							}
							im_pop_frame();
						}
					}
					im_pop_frame();
				}
			}
			im_pop_frame();
		}
	}
	
	/* In special cases you can specify the next ID to be used */
	im_set_next_id(333l);
	im_push_frame(IM_FILL);
	
	TEST_ASSERT_EQUAL_UINT32(333l, im_get_element()->id);
}

TEST(core_layout, limits) {
	/* We can insert a total of 2 elements into this one. Further push_frame calls will return FALSE */
	im_push_frame_insets_params(IM_FILL, insets_zero(), (im_layout_params_t) {
    0,
    .limit.total = 2,
    .options = IM_DEFAULT_LAYOUT_FLAGS
  });
	
	if (im_push_frame(IM_FILL)) { im_pop_frame(); }
	if (im_push_frame(IM_FILL)) { im_pop_frame(); }
	
	if (im_push_frame(IM_FILL)) { TEST_FAIL_MESSAGE("Limit exceeded"); }
	
	im_pop_frame();
}

TEST(core_layout, insets) {
	/* We are changing root frame insets directly. Insets only apply to the children */
	im_get_element()->insets = insets_uniform(10);
	
	/*
	The next frame should be smaller by 10 units on each side as set by its parent.
	`IM_FILL` instructs the frame to calculate its size automatically based on where it's being inserted.
	*/
	im_push_frame_insets(IM_FILL, insets_make(50, 0, 0, 0));
	
	/*
	Top left origin (X, Y) remains at zero because padding doesn't change
	the coordinates directly - it's applied when calculating the absolute (on-screen)
	frame when rendering. Width and height, however, already take padding(s) into account.
	*/
	TEST_ASSERT_EQUAL_FRAME(frame_make(0, 0, 620, 460), im_get_element()->frame);
	TEST_ASSERT_EQUAL_FRAME(frame_make(10, 10, 620, 460), im_get_absolute_frame());
	
	/* Push another frame. This time there's padding only on the left as set by the previously pushed frame */
	im_push_frame(IM_FILL);
	
	TEST_ASSERT_EQUAL_FRAME(frame_make(0, 0, 570, 460), im_get_element()->frame);
	TEST_ASSERT_EQUAL_FRAME(frame_make(60, 10, 570, 460), im_get_absolute_frame());
	
	/* Another relative frame, this time off-set from the origin */
	im_push_frame(frame_make(30, 40, 100, 100));
	
	TEST_ASSERT_EQUAL_FRAME(frame_make(30, 40, 100, 100), im_get_element()->frame);
	TEST_ASSERT_EQUAL_FRAME(frame_make(90, 50, 100, 100), im_get_absolute_frame());
	
	/*
	Here's what our layout looks like:

	┌0x0─(Root)──────────────────────640w┐   
	│                 10                 │   
	│  ┌0x0─(First)───────────────620w┐  │
	│  │                  0           │  │ 
	│  │        ┌0x0─(Second)───570w┐ │  │   
	│  │        │                   │ │  │   
	│  │        │   ┌30x40──┐       │ │  │   
	│  │        │   │       │       │0│  │   
	│10│ <-50-> │   │       │       │ │10│   
	│  │        │   │       │       4 │  │   
	│  │        │   └───────┘       6 │  │   
	│  │        │                   0 4  │   
	│  │        └───────────────────┘ 6  │
	│  │                  0           0  4
	│  └──────────────────────────────┘  8   
	│                 10                 0   
	└────────────────────────────────────┘
	*/
}

TEST(core_layout, overlay) {
	/* Frames don't have to be nested to be overlap or appear to be contained */
	
	/*
	In this case relative and absolute frames are the same
	because they're directly in root's coordinate system.
	*/
	im_push_frame(frame_make(50, 50, 540, 380));
	
	TEST_ASSERT_EQUAL_FRAME(frame_make(50, 50, 540, 380), im_get_element()->frame);
	TEST_ASSERT_EQUAL_FRAME(frame_make(50, 50, 540, 380), im_get_absolute_frame());
	
	im_pop_frame();
	
	im_push_frame(frame_make(100, 100, 440, 280));
	
	TEST_ASSERT_EQUAL_FRAME(frame_make(100, 100, 440, 280), im_get_element()->frame);
	TEST_ASSERT_EQUAL_FRAME(frame_make(100, 100, 440, 280), im_get_absolute_frame());
	
	im_pop_frame();
	
	/*
	On screen these 2 frames would appear nested but they're not. They could be
	modal windows for example:

	╔═════════════════════════════════╗  
	║                                 ║  
	║  ┌[Window]────────────────[x]┐  ║  
	║  │                           │  ║  
	║  │    ┌[Window]──────[x]┐    │  ║  
	║  │    │                 │    │  ║  
	║  │    │                 │    │  ║  
	║  │    │                 │    │  ║  
	║  │    │                 │    │  ║  
	║  │    │                 │    │  ║  
	║  │    └─────────────────┘    │  ║  
	║  │                           │  ║  
	║  └───────────────────────────┘  ║  
	║                                 ║  
	╚═════════════════════════════════╝ 
	*/
}

TEST(core_layout, culling) {
	/*
	Frames that are wholly outside of visible area are not added.
	`im_push_frame_*` functions return a BOOL for that. If the frame
	is added successfully you are expected to pop it at some point.
	*/
	
	/* This one is wholly outside the parent's top edge */
	if (im_push_frame(frame_make(0, -50, 100, 50))) {
		TEST_FAIL_MESSAGE("Frame should have been culled");
	}
	
	/* This one is wholly outside the parent's right edge */
	if (im_push_frame(frame_make(640, 0, 100, 50))) {
		TEST_FAIL_MESSAGE("Frame should have been culled");
	}
	
	/* This one partially intersects the parent */
	if (im_push_frame(frame_make(0, -25, 100, 50))) {
		im_pop_frame();
	} else {
		TEST_FAIL_MESSAGE("Frame should NOT have been culled");
	}
	
	/* Culling is enabled by default. We can disable it (for current element) */
	im_disable_culling();
	
	/* Even though it lays outside the parent, it's still added */
	if (im_push_frame(frame_make(0, -50, 100, 25))) {
		im_pop_frame();
	} else {
		TEST_FAIL_MESSAGE("Frame should NOT have been culled");
	}
}

TEST(core_layout, vstack_layout) {
	/* Pushing a stack that lays out frames vertically with a 10pt spacing */
	if (!im_push_frame_function(&im_default_layout_builder, IM_FILL, insets_zero(), (im_layout_params_t) {
    0,
    .axis = VERTICAL,
    .spacing = 10,
		.limit.vertical = 2,
    .options = IM_DEFAULT_LAYOUT_FLAGS
  })) {
		TEST_FAIL_MESSAGE("Unable to add layout builder frame");
	}
	
	/* Width is calculated, but height is fixed at 50pt */
	im_push_frame(IM_FILL_H(50));
	TEST_ASSERT_EQUAL_FRAME(frame_make(0, 0, 640, 50), im_get_element()->frame);
	im_pop_frame();
	
	im_push_frame(IM_FILL_H(100));
	TEST_ASSERT_EQUAL_FRAME(frame_make(0, 50+10, 640, 100), im_get_element()->frame);
	im_pop_frame();
	
	TEST_ASSERT_EQUAL_INT(170, im_get_element()->_layout_params._vertical_position);
	
	if (im_push_frame(IM_FILL)) { TEST_FAIL_MESSAGE("Vertical limit exceeded"); }
	
	im_pop_frame(); /* Not really necessary in testing, but.. */
}

TEST(core_layout, hstack_layout) {
	/* Pushing a stack that lays out frames horizontally with no spacing, but everything is inset by 10pt */
	if (!im_push_frame_function(&im_default_layout_builder, IM_FILL, insets_uniform(10), (im_layout_params_t) {
    0,
    .axis = HORIZONTAL,
    .spacing = 0,
    .options = IM_DEFAULT_LAYOUT_FLAGS
  })) {
		TEST_FAIL_MESSAGE("Unable to add layout builder frame");
	}
	
	/* Width is calculated, but height is fixed at 50pt */
	im_push_frame(IM_FILL_W(200));
	TEST_ASSERT_EQUAL_FRAME(frame_make(0, 0, 200, 480-2*10), im_get_element()->frame);
	im_pop_frame();
	
	im_push_frame(IM_FILL_W(100));
	TEST_ASSERT_EQUAL_FRAME(frame_make(200, 0, 100, 480-2*10), im_get_element()->frame);
	im_pop_frame();
	
	TEST_ASSERT_EQUAL_INT(300, im_get_element()->_layout_params._horizontal_position);
	
	im_pop_frame(); /* Not really necessary in testing, but.. */
}

/*
	- Grid is the same stack layout builder but with both axis enabled.
	- Grid will start adding frames horizontally/vertically, until the position exceeds
	  the width/height or when the next proposed frame can't fit.
	  Then the position moves to the next row/column.
*/

TEST(core_layout, grid_with_fixed_rows_and_columns) {
	/*
	We are specifying a number of rows and columns - this will tell
	how large each child needs to be by default (we *can* override).
	Here it's a 5x5 grid, meaning on our 640x480 screen,
	each cell would be 128x96.
	*/
	if (!im_push_frame_function(&im_default_layout_builder, IM_FILL, insets_zero(), (im_layout_params_t) {
    0,
    .axis = HORIZONTAL | VERTICAL,
    .spacing = 0,
		.columns = 5,
		.rows = 5,
    .options = IM_DEFAULT_LAYOUT_FLAGS
  })) {
		TEST_FAIL_MESSAGE("Unable to add layout builder frame");
	}
	
	int i;
	for (i = 0; i < 25; ++i) {
		int row = (i / 5);
		int column = i - (row * 5);
		im_push_frame(IM_FILL);
		TEST_ASSERT_EQUAL_FRAME(frame_make(128*column, 96*row, 128, 96), im_get_element()->frame);
		im_pop_frame();
	}

	TEST_ASSERT_EQUAL_INT(0, im_get_element()->_layout_params._horizontal_position);
	TEST_ASSERT_EQUAL_INT(480, im_get_element()->_layout_params._vertical_position);
}

TEST(core_layout, grid_with_fixed_cell_size) {
	/*
	Here we are defining a grid where each cell's width and height is fixed.
	If columns and rows are not set, there will be as many cells horizontally
	as could be fitted, in this case 640 / 200 = 3 and the remaining space will
	be unused. Same vertically (480 / 200) = 2, 6 in total, looking something like this:
	
	┌─────────────────────────────────┐
	│┌────────┐┌────────┐┌────────┐...│
	││        ││        ││        │...│
	││        ││        ││        │...│
	││        ││        ││        │...│
	│└────────┘└────────┘└────────┘...│
	│┌────────┐┌────────┐┌────────┐...│
	││        ││        ││        │...│
	││        ││        ││        │...│
	││        ││        ││        │...│
	│└────────┘└────────┘└────────┘...│
	│.................................│
	└─────────────────────────────────┘
	*/
	if (!im_push_frame_function(&im_default_layout_builder, IM_FILL, insets_zero(), (im_layout_params_t) {
    0,
    .axis = HORIZONTAL | VERTICAL,
    .width = 200,
		.height = 200,
    .options = IM_DEFAULT_LAYOUT_FLAGS
  })) {
		TEST_FAIL_MESSAGE("Unable to add layout builder frame");
	}
	
	int i;
	
	/* First row */
	for (i = 0; i < 3; ++i) {
		im_push_frame(IM_FILL);
		im_pop_frame();
	}
	
	TEST_ASSERT_EQUAL_INT(600, im_get_element()->_layout_params._horizontal_position);
	
	/* Second row */
	for (i = 0; i < 3; ++i) {
		im_push_frame(IM_FILL);
		TEST_ASSERT_EQUAL_INT(200, im_get_element()->frame.y);
		im_pop_frame();
	}
	
	TEST_ASSERT_EQUAL_INT(200, im_get_element()->_layout_params._vertical_position);
}

TEST(core_layout, grid_with_varying_cell_size) {
	/*
	Third option is to specify a size for each of the cells at insertion time.
	Then, again depending on the remaining space, cell will be inserted into the
	current row or pushed to the next. In addition, you can still specify the number
	of rows and columns, and these will now be used to limit number of cells on each axis.
	*/
	if (!im_push_frame_function(&im_default_layout_builder, IM_FILL, insets_zero(), (im_layout_params_t) {
    0,
    .axis = HORIZONTAL | VERTICAL,
		.limit.horizontal = 3,
    .options = IM_DEFAULT_LAYOUT_FLAGS
  })) {
		TEST_FAIL_MESSAGE("Unable to add layout builder frame");
	}

	/* Add 3 elements with increasing width. They should fit on the first row */
	im_push_frame(frame_make(0, 0, 100, 160)); /* (1) */
	TEST_ASSERT_EQUAL_INT(0, im_get_element()->frame.x);
	im_pop_frame();
	
	TEST_ASSERT_EQUAL_INT(100, im_get_element()->_layout_params._horizontal_position);
	
	im_push_frame(frame_make(0, 0, 200, 160)); /* (2) */
	TEST_ASSERT_EQUAL_INT(100, im_get_element()->frame.x);
	im_pop_frame();
	
	TEST_ASSERT_EQUAL_INT(300, im_get_element()->_layout_params._horizontal_position);
	
	im_push_frame(frame_make(0, 0, 300, 160)); /* (3) */
	TEST_ASSERT_EQUAL_INT(300, im_get_element()->frame.x);
	im_pop_frame();
	
	TEST_ASSERT_EQUAL_INT(600, im_get_element()->_layout_params._horizontal_position);
	
	/*
	Let's try to insert another cell that should fit width wise,
	but the grid would exceed the number of horizontal elements,
	so it's pushed onto the next row.
	*/
	im_push_frame(frame_make(0, 0, 40, 160));
	TEST_ASSERT_EQUAL_INT(0, 		im_get_element()->frame.x);
	TEST_ASSERT_EQUAL_INT(160, 	im_get_element()->frame.y);
	im_pop_frame();
	
	/* This one should be inserted normally onto the second row */
	im_push_frame(frame_make(0, 0, 400, 160)); /* (5) */
	TEST_ASSERT_EQUAL_INT(40, 	im_get_element()->frame.x);
	TEST_ASSERT_EQUAL_INT(160,	im_get_element()->frame.y);
	im_pop_frame();
	
	/* Spacer fills the remaining space on the second row. Internally this is just a frame push + pop */
	im_spacer(IM_FILL_CONSTANT /* === 0 */);
	
	/* Insert an element to fill all the space on the third row */
	im_push_frame(IM_FILL); /* (6) */
	TEST_ASSERT_EQUAL_INT(640, 	im_get_element()->frame.w);
	TEST_ASSERT_EQUAL_INT(160, 	im_get_element()->frame.h);
	im_pop_frame();
	
	/*
	For visualisation:
	┌──────────────────────┐
	│┌──┐┌─────┐┌───────┐..│
	││1 ││  2  ││   3   │..│ <- (4) could go here but we've
	│└──┘└─────┘└───────┘..│    set a limit on 3 columns, or
	│┌─┐┌────────────┐┌   ┐│    3 elements per row more precisely
	││4││    5       │     │
	│└─┘└────────────┘└   ┘│
	│┌────────────────────┐│
	││         6          ││
	│└────────────────────┘│
	└──────────────────────┘
	*/
}

TEST(core_layout, grid_with_down_direction) {
	/*
	Grids support horizontal (default) and vertical layout direction. In vertical mode,
	instead of filling and adding rows, columns are filled and added instead. Otherwise
	they behave the same.
	*/
	if (!im_push_frame_function(&im_default_layout_builder, IM_FILL, insets_zero(), (im_layout_params_t) {
    0,
    .axis = HORIZONTAL | VERTICAL,
		.direction = DIR_DOWN,
    .options = IM_DEFAULT_LAYOUT_FLAGS
  })) {
		TEST_FAIL_MESSAGE("Unable to add layout builder frame");
	}
	
	/* Add 3 elements with increasing height and width. They should fit in the first column */
	im_push_frame(frame_make(0, 0, 100, 120)); /* (1) */
	TEST_ASSERT_EQUAL_INT(0,		im_get_element()->frame.y);
	im_pop_frame();
	
	TEST_ASSERT_EQUAL_INT(120,	im_get_element()->_layout_params._vertical_position);
	
	im_push_frame(frame_make(0, 0, 150, 160)); /* (2) */
	TEST_ASSERT_EQUAL_INT(120,	im_get_element()->frame.y);
	im_pop_frame();
	
	TEST_ASSERT_EQUAL_INT(280, 	im_get_element()->_layout_params._vertical_position);
	
	im_push_frame(frame_make(0, 0, 200, 200)); /* (3) */
	TEST_ASSERT_EQUAL_INT(280, 	im_get_element()->frame.y);
	im_pop_frame();
	
	/* No remaining space vertically - position moves back to the top and to the next column */
	TEST_ASSERT_EQUAL_INT(200,	im_get_element()->_layout_params._horizontal_position);
	TEST_ASSERT_EQUAL_INT(0,		im_get_element()->_layout_params._vertical_position);
	
	/*
	Without anything else configured on the grid, the next element will fill
	the remaining space on the right.
	*/
	im_push_frame(IM_FILL); /* (4) */
	TEST_ASSERT_EQUAL_FRAME(frame_make(200, 0, 440, 480), im_get_element()->frame);
	im_pop_frame();
	
	TEST_ASSERT_EQUAL_INT(640, 	im_get_element()->_layout_params._horizontal_position);
	
	/*
	For visualisation:
	┌────────────────────────┐
	│┌───┐....┌─────────────┐│
	││ 1 │....│             ││
	│└───┘....│             ││
	│┌─────┐..│             ││
	││  2  │..│             ││
	│└─────┘..│      4      ││
	│┌───────┐│             ││
	││       ││             ││
	││   3   ││             ││
	││       ││             ││
	│└───────┘└─────────────┘│
	└────────────────────────┘
	*/
}

TEST(core_layout, vstack_scroll) {
	/* Any element can be made scrollable, but it makes most sense for stacks/grids */
	if (!im_push_frame_function(&im_default_layout_builder, IM_FILL, insets_zero(), (im_layout_params_t) {
    0,
    .axis = VERTICAL,
		.height = 100,
    .options = IM_DEFAULT_LAYOUT_FLAGS
  })) {
		TEST_FAIL_MESSAGE("Unable to add layout builder frame");
	}
	
	/* Scrolling is not enabled by default */
	TEST_ASSERT_NULL(im_get_element()->_scroll_state);
	TEST_ASSERT_FALSE(im_get_element()->_clipped);
	
	im_enable_scroll(NULL);
	
	/* Scrolling also enables clipping */
	TEST_ASSERT_TRUE(im_get_element()->_clipped);
	
	im_scroll_state_t *scroll = im_get_scroll_state();
	
	TEST_ASSERT_EQUAL_VEC2(vec2_zero(), scroll->offset);
	TEST_ASSERT_EQUAL_VEC2(vec2_zero(), scroll->content_size);
	
	scroll->offset.y = 220;
	
	/* Let's add some content to the stack */
	for (int i = 0; i < 10; ++i) {
		if (im_push_frame(IM_FILL)) {
			/* Elements should be offset by scroll amount on the Y axis */
			TEST_ASSERT_EQUAL_INT(i*100-220, im_get_element()->frame.y);
			im_pop_frame();
		}
	}
	
	TEST_ASSERT_EQUAL_VEC2(vec2_make(640, 100*10), scroll->content_size);
}

TEST(core_layout, clipping) {
	/* Clipping is partially a graphical feature implemented in the backend,
	   but the layout elements also calculate a relative frame that's been clipped.
		 
		 Clipping is OFF by default, but every layout element is eventually clipped
		 against the root element (the screen itself) because things are clipped
		 automatically by most renderers when drawing out of bounds */ 
		 
	/* An element that's partially outside the root bounds gets clipped */
	im_push_frame(frame_make(100, -100, 440, 200));
	TEST_ASSERT_EQUAL_FRAME(frame_make(100, 0, 440, 100), im_get_element()->clipped_frame);
	im_pop_frame();
	
	/* Some element filling the whole root */
	im_push_frame(IM_FILL);
			
	/* Clipping is now turned ON for the current layout element */
	im_enable_clipping();
	
	im_push_frame(frame_make(600, 400, 100, 100));
	
	TEST_ASSERT_EQUAL_FRAME(frame_make(600, 400, 40, 80), im_get_element()->clipped_frame);
		
		im_push_frame(frame_make(30, 70, 20, 20));
		TEST_ASSERT_EQUAL_FRAME(frame_make(30, 70, 10, 10), im_get_element()->clipped_frame);
		im_pop_frame();
		
	im_pop_frame();
	
	
	im_push_frame(frame_make(-75, -50, 200, 200));
	
	TEST_ASSERT_EQUAL_FRAME(frame_make(0, 0, 125, 150), im_get_element()->clipped_frame);
		
		im_push_frame(frame_make(0, 0, 100, 100));
		TEST_ASSERT_EQUAL_FRAME(frame_make(75, 50, 25, 50), im_get_element()->clipped_frame);
		im_pop_frame();
		
		im_push_frame(frame_make(-25, -50, 100, 100));
		TEST_ASSERT_EQUAL_FRAME(frame_make(75, 50, 0, 0), im_get_element()->clipped_frame);
		im_pop_frame();
		
	im_pop_frame();
	
	im_push_frame(frame_make(100, 100, 440, 280));
	im_enable_clipping();
	
		im_push_frame(frame_make(-10, -50, 460, 100));
		TEST_ASSERT_EQUAL_FRAME(frame_make(0, 0, 440, 50), im_get_element()->clipped_frame);
		im_pop_frame();
	
	im_pop_frame();

	im_pop_frame();
}

TEST_GROUP_RUNNER(core_layout) {
  RUN_TEST_CASE(core_layout, basic_check);
  RUN_TEST_CASE(core_layout, push_pop);
  RUN_TEST_CASE(core_layout, identifiers);
  RUN_TEST_CASE(core_layout, limits);
  RUN_TEST_CASE(core_layout, insets);
  RUN_TEST_CASE(core_layout, overlay);
  RUN_TEST_CASE(core_layout, culling);
  RUN_TEST_CASE(core_layout, vstack_layout);
  RUN_TEST_CASE(core_layout, hstack_layout);
  RUN_TEST_CASE(core_layout, grid_with_fixed_rows_and_columns);
  RUN_TEST_CASE(core_layout, grid_with_fixed_cell_size);
  RUN_TEST_CASE(core_layout, grid_with_varying_cell_size);
  RUN_TEST_CASE(core_layout, grid_with_down_direction);
  RUN_TEST_CASE(core_layout, vstack_scroll);
  RUN_TEST_CASE(core_layout, clipping);
}
