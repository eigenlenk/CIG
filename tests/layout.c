#include "unity.h"
#include "fixture.h"
#include "imgui.h"

static void assert_frame_equal(const frame_t exp, const frame_t act, unsigned int line) {
	char message[48];
	sprintf(message, "(%d, %d, %d, %d) != (%d, %d, %d, %d)", exp.x, exp.y, exp.w, exp.h, act.x, act.y, act.w, act.h);
	TEST_ASSERT_MESSAGE(frame_cmp(exp, act), message);
}

#define TEST_ASSERT_EQUAL_FRAME(expected, actual) assert_frame_equal(expected, actual, __LINE__)

TEST_GROUP(layout);

TEST_SETUP(layout) {
	/* Begin laying out a screen that's 640 by 480 */
	im_begin_layout(NULL, frame_make(0, 0, 640, 480));
}

TEST_TEAR_DOWN(layout) {
	im_end_layout();
}

/* (( TEST CASES )) */

TEST(layout, basic_check) {
	/* Nothing really to test here, just checking nothing crashes =) */
	TEST_ASSERT_EQUAL_FRAME(frame_make(0, 0, 640, 480), im_current_element()->frame);
}

TEST(layout, push_pop) {
	TEST_ASSERT_EQUAL(im_depth(), 1); /* Just the root */
	
	im_push_frame(IM_FILL);
		im_push_frame(IM_FILL);
			im_push_frame(IM_FILL);
			
				TEST_ASSERT_EQUAL(im_depth(), 4);
				
			im_pop_frame();
		im_pop_frame();
	
		TEST_ASSERT_EQUAL(im_depth(), 2);

		im_push_frame(IM_FILL);
		
			TEST_ASSERT_EQUAL(im_depth(), 3);
			
		im_pop_frame();
	im_pop_frame();
	
	TEST_ASSERT_EQUAL(im_depth(), 1); /* Just the root again */
}

TEST(layout, insets) {
	/* We are changing root frame insets directly. Insets only apply to the children */
	im_current_element()->insets = insets_uniform(10);
	
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
	TEST_ASSERT_EQUAL_FRAME(frame_make(0, 0, 620, 460), im_current_element()->frame);
	TEST_ASSERT_EQUAL_FRAME(frame_make(10, 10, 620, 460), im_absolute_frame());
	
	/* Push another frame. This time there's padding only on the left as set by the previously pushed frame */
	im_push_frame(IM_FILL);
	
	TEST_ASSERT_EQUAL_FRAME(frame_make(0, 0, 570, 460), im_current_element()->frame);
	TEST_ASSERT_EQUAL_FRAME(frame_make(60, 10, 570, 460), im_absolute_frame());
	
	/*
	Here's what our layout looks like:

	┌0x0─(Root)──────────────────────640w┐   
	│                 10                 │   
	│  ┌0x0─(First)───────────────620w┐  │
	│  │                  0           │  │ 
	│  │        ┌0x0─(Second)───570w┐ │  │   
	│  │        │                   │ │  │   
	│  │        │                   │ │  │   
	│  │        │                   │0│  │   
	│10│ <-50-> │                   │ │10│   
	│  │        │                   4 │  │   
	│  │        │                   6 │  │   
	│  │        │                   0 4  │   
	│  │        └───────────────────┘ 6  │
	│  │                  0           0  4
	│  └──────────────────────────────┘  8   
	│                 10                 0   
	└────────────────────────────────────┘
	*/
}

TEST(layout, culling) {
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
	
	/* Culling is enabled by default. We can disable it. */
	im_disable_culling();
	
	/* Even though it lays outside the parent, it's still added */
	if (im_push_frame(frame_make(0, -50, 100, 25))) {
		im_pop_frame();
	} else {
		TEST_FAIL_MESSAGE("Frame should NOT have been culled");
	}
}

TEST(layout, vstack_layout) {
	/* Pushing a stack that lays out frames vertically with a 10pt spacing */
	if (!im_push_frame_builder(IM_FILL, insets_zero(), &im_stack_layout_builder, (im_layout_params_t) {
    0,
    .axis = VERTICAL,
    .spacing = 10,
    .options = IM_DEFAULT_LAYOUT_FLAGS
  })) {
		TEST_FAIL_MESSAGE("Unable to add layout builder frame");
	}
	
	/* Width is calculated, but height is fixed at 50pt */
	im_push_frame(IM_FILL_H(50));
	TEST_ASSERT_EQUAL_FRAME(frame_make(0, 0, 640, 50), im_current_element()->frame);
	im_pop_frame();
	
	im_push_frame(IM_FILL_H(100));
	TEST_ASSERT_EQUAL_FRAME(frame_make(0, 50+10, 640, 100), im_current_element()->frame);
	im_pop_frame();
	
	TEST_ASSERT_EQUAL_INT(170, im_current_element()->_layout_params._vertical_position);
	
	im_pop_frame(); /* Not really necessary in testing, but.. */
}

TEST(layout, hstack_layout) {
	/* Pushing a stack that lays out frames horizontally with no spacing, but everything is inset by 10pt */
	if (!im_push_frame_builder(IM_FILL, insets_uniform(10), &im_stack_layout_builder, (im_layout_params_t) {
    0,
    .axis = HORIZONTAL,
    .spacing = 0,
    .options = IM_DEFAULT_LAYOUT_FLAGS
  })) {
		TEST_FAIL_MESSAGE("Unable to add layout builder frame");
	}
	
	/* Width is calculated, but height is fixed at 50pt */
	im_push_frame(IM_FILL_W(200));
	TEST_ASSERT_EQUAL_FRAME(frame_make(0, 0, 200, 480-2*10), im_current_element()->frame);
	im_pop_frame();
	
	im_push_frame(IM_FILL_W(100));
	TEST_ASSERT_EQUAL_FRAME(frame_make(200, 0, 100, 480-2*10), im_current_element()->frame);
	im_pop_frame();
	
	TEST_ASSERT_EQUAL_INT(300, im_current_element()->_layout_params._horizontal_position);
	
	im_pop_frame(); /* Not really necessary in testing, but.. */
}

/*
	- Grid is the same stack layout builder but with both axis enabled.
	- Grid will start adding frames horizontally/vertically, until the position exceeds
	  the width/height or when the next proposed frame can't fit.
	  Then the position moves to the next row/column.
*/

TEST(layout, grid_with_fixed_rows_and_columns) {
	/*
	We are specifying a number of rows and columns - this will tell
	how large each child needs to be by default (we *can* override).
	Here it's a 5x5 grid, meaning on our 640x480 screen,
	each cell would be 128x96.
	*/
	if (!im_push_frame_builder(IM_FILL, insets_zero(), &im_stack_layout_builder, (im_layout_params_t) {
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
		TEST_ASSERT_EQUAL_FRAME(frame_make(128*column, 96*row, 128, 96), im_current_element()->frame);
		im_pop_frame();
	}

	TEST_ASSERT_EQUAL_INT(0, im_current_element()->_layout_params._horizontal_position);
	TEST_ASSERT_EQUAL_INT(480, im_current_element()->_layout_params._vertical_position);
}

TEST(layout, grid_with_fixed_cell_size) {
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
	if (!im_push_frame_builder(IM_FILL, insets_zero(), &im_stack_layout_builder, (im_layout_params_t) {
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
	
	TEST_ASSERT_EQUAL_INT(600, im_current_element()->_layout_params._horizontal_position);
	
	/* Second row */
	for (i = 0; i < 3; ++i) {
		im_push_frame(IM_FILL);
		TEST_ASSERT_EQUAL_INT(200, im_current_element()->frame.y);
		im_pop_frame();
	}
	
	TEST_ASSERT_EQUAL_INT(200, im_current_element()->_layout_params._vertical_position);
}

TEST(layout, grid_with_varying_cell_size) {
	/*
	Third option is to specify a size for each of the cells at insertion time.
	Then, again depending on the remaining space, cell will be inserted into the
	current row or pushed to the next. In addition, you can still specify the number
	of rows and columns, and these will now be used to limit number of cells on each axis.
	*/
	if (!im_push_frame_builder(IM_FILL, insets_zero(), &im_stack_layout_builder, (im_layout_params_t) {
    0,
    .axis = HORIZONTAL | VERTICAL,
    .options = IM_DEFAULT_LAYOUT_FLAGS
  })) {
		TEST_FAIL_MESSAGE("Unable to add layout builder frame");
	}

	/* Add 3 elements with increasing width. They should fit on the first row */
	im_push_frame(frame_make(0, 0, 100, 160));
	TEST_ASSERT_EQUAL_INT(0, im_current_element()->frame.x);
	im_pop_frame();
	
	TEST_ASSERT_EQUAL_INT(100, im_current_element()->_layout_params._horizontal_position);
	
	im_push_frame(frame_make(0, 0, 200, 160));
	TEST_ASSERT_EQUAL_INT(100, im_current_element()->frame.x);
	im_pop_frame();
	
	TEST_ASSERT_EQUAL_INT(300, im_current_element()->_layout_params._horizontal_position);
	
	im_push_frame(frame_make(0, 0, 300, 160));
	TEST_ASSERT_EQUAL_INT(300, im_current_element()->frame.x);
	im_pop_frame();
	
	TEST_ASSERT_EQUAL_INT(600, im_current_element()->_layout_params._horizontal_position);
	
	/*
	Let's try to insert another cell that should fit width-wise,
	but the grid would exceed the number of columns we set, so it's
	pushed onto the next row.
	*/
	// im_push_frame(frame_make(0, 0, 40, 160));
	// TEST_ASSERT_EQUAL_INT(0, 		im_current_element()->frame.x);
	// TEST_ASSERT_EQUAL_INT(160, 	im_current_element()->frame.y);
	// im_pop_frame();
	
	/* Fifth one should be inserted normally onto the second row */
	im_push_frame(frame_make(0, 0, 400, 160));
	TEST_ASSERT_EQUAL_INT(0, 		im_current_element()->frame.x);
	TEST_ASSERT_EQUAL_INT(160,	im_current_element()->frame.y);
	im_pop_frame();
	
	/* Fills the remaining space on the second row. Internally this is just a frame push + pop */
	im_insert_spacer(IM_FILL_CONSTANT /* === 0 */);
	
	/* Insert a sixth cell to fill all the space on the third row (since it doesn't fit on second) */
	im_push_frame(IM_FILL);
	TEST_ASSERT_EQUAL_INT(640, 	im_current_element()->frame.w);
	TEST_ASSERT_EQUAL_INT(160, 	im_current_element()->frame.h);
	im_pop_frame();
	
	/*
	For visualisation:
	┌─────────────────────┐
	│┌──┐┌─────┐┌───────┐.│
	││1 ││  2  ││   3   │.│
	│└──┘└─────┘└───────┘.│
	│┌─┐┌────────────┐┌  ┐│
	││4││    5       │    │
	│└─┘└────────────┘└  ┘│
	│┌───────────────────┐│
	││         6         ││
	│└───────────────────┘│
	└─────────────────────┘
	*/
}

TEST(layout, grid_with_down_direction) {
	/*
	Grids support horizontal (default) and vertical layout direction. In vertical mode,
	instead of filling and adding rows, columns are filled and added instead. Otherwise
	they behave the same.
	*/
	if (!im_push_frame_builder(IM_FILL, insets_zero(), &im_stack_layout_builder, (im_layout_params_t) {
    0,
    .axis = HORIZONTAL | VERTICAL,
		.direction = DIR_DOWN,
    .options = IM_DEFAULT_LAYOUT_FLAGS
  })) {
		TEST_FAIL_MESSAGE("Unable to add layout builder frame");
	}
	
	/* Add 3 elements with increasing height and width. They should fit in the first column */
	im_push_frame(frame_make(0, 0, 100, 120)); /* (1) */
	TEST_ASSERT_EQUAL_INT(0,		im_current_element()->frame.y);
	im_pop_frame();
	
	TEST_ASSERT_EQUAL_INT(120,	im_current_element()->_layout_params._vertical_position);
	
	im_push_frame(frame_make(0, 0, 150, 160)); /* (2) */
	TEST_ASSERT_EQUAL_INT(120,	im_current_element()->frame.y);
	im_pop_frame();
	
	TEST_ASSERT_EQUAL_INT(280, 	im_current_element()->_layout_params._vertical_position);
	
	im_push_frame(frame_make(0, 0, 200, 200)); /* (3) */
	TEST_ASSERT_EQUAL_INT(280, 	im_current_element()->frame.y);
	im_pop_frame();
	
	/* No remaining space vertically - position moves back to the top and to the next column */
	TEST_ASSERT_EQUAL_INT(200,	im_current_element()->_layout_params._horizontal_position);
	TEST_ASSERT_EQUAL_INT(0,		im_current_element()->_layout_params._vertical_position);
	
	/*
	Without anything else configured on the grid, the next element will fill
	the remaining space on the right.
	*/
	im_push_frame(IM_FILL); /* (4) */
	TEST_ASSERT_EQUAL_FRAME(frame_make(200, 0, 440, 480), im_current_element()->frame);
	im_pop_frame();
	
	TEST_ASSERT_EQUAL_INT(640, 	im_current_element()->_layout_params._horizontal_position);
	
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

TEST_GROUP_RUNNER(layout) {
  RUN_TEST_CASE(layout, basic_check);
  RUN_TEST_CASE(layout, push_pop);
  RUN_TEST_CASE(layout, insets);
  RUN_TEST_CASE(layout, culling);
  RUN_TEST_CASE(layout, vstack_layout);
  RUN_TEST_CASE(layout, hstack_layout);
  RUN_TEST_CASE(layout, grid_with_fixed_rows_and_columns);
  RUN_TEST_CASE(layout, grid_with_fixed_cell_size);
  RUN_TEST_CASE(layout, grid_with_varying_cell_size);
  RUN_TEST_CASE(layout, grid_with_down_direction);
}
