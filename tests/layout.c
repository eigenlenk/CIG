#include "unity.h"
#include "fixture.h"
#include "imgui.h"

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
	/* Nothing really to assert here, just checking nothing crashes =) */
	TEST_ASSERT(frame_cmp(im_current_element()->frame, frame_make(0, 0, 640, 480)));
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
	TEST_ASSERT(frame_cmp(im_current_element()->frame, frame_make(0, 0, 620, 460)));
	TEST_ASSERT(frame_cmp(im_absolute_frame(), frame_make(10, 10, 620, 460)));
	
	/* Push another frame. This time there's padding only on the left as set by the previously pushed frame */
	im_push_frame(IM_FILL);
	
	TEST_ASSERT(frame_cmp(im_current_element()->frame, frame_make(0, 0 ,570, 460)));
	TEST_ASSERT(frame_cmp(im_absolute_frame(), frame_make(60, 10, 570, 460)));
	
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
	`im_push_frame_*` functions return a BOOL for that.
	*/
	
	/* This one is wholly outside the parent's top edge */
	if (im_push_frame(frame_make(0, -50, 100, 50))) {
		TEST_FAIL_MESSAGE("Frame should have been culled!");
	}
	
	/* This one is wholly outside the parent's right edge */
	if (im_push_frame(frame_make(640, 0, 100, 50))) {
		TEST_FAIL_MESSAGE("Frame should have been culled!");
	}
	
	/* This one partially intersects the parent */
	if (im_push_frame(frame_make(0, -25, 100, 50))) {
		im_pop_frame();
	} else {
		TEST_FAIL_MESSAGE("Frame should NOT have been culled!");
	}
	
	/* Culling is enabled by default. We can disable it. */
	im_disable_culling();
	
	/* Even though it lays outside the parent, it's still added */
	if (im_push_frame(frame_make(0, -50, 100, 25))) {
		im_pop_frame();
	} else {
		TEST_FAIL_MESSAGE("Frame should NOT have been culled!");
	}
}

TEST_GROUP_RUNNER(layout) {
  RUN_TEST_CASE(layout, basic_check);
  RUN_TEST_CASE(layout, push_pop);
  RUN_TEST_CASE(layout, insets);
  RUN_TEST_CASE(layout, culling);
}
