#include "unity.h"
#include "fixture.h"
#include "imgui.h"
#include "asserts.h"

TEST_GROUP(core_input);

TEST_SETUP(core_input) {}
TEST_TEAR_DOWN(core_input) {}

static void begin() {
	im_begin_layout(NULL, frame_make(0, 0, 640, 480));
}

static void end() {
	im_end_layout();
}

TEST(core_input, hover_and_press) {
	for (int i = 0; i < 2; ++i) {
		begin();
		
		im_set_input_state(vec2_make(50, 50), i == 1 ? IM_MOUSE_BUTTON_LEFT : 0);
		
		im_push_frame(frame_make(0, 0, 100, 100));
		
		im_enable_interaction(); /* This element now tracks mouse inputs */
		
		if (i == 1) {
			TEST_ASSERT_TRUE(im_hovered());
			TEST_ASSERT_TRUE(im_pressed(IM_MOUSE_BUTTON_ANY, IM_MOUSE_PRESS_INSIDE));
		}
		
		im_pop_frame();
		
		end();
	}
}

TEST(core_input, overlapping_hover_and_press) {
	for (int i = 0; i < 2; ++i) {
		begin();
		
		im_set_input_state(vec2_make(75, 75), i == 1 ? IM_MOUSE_BUTTON_LEFT : 0);
		
		im_push_frame(frame_make(0, 0, 100, 100));
		
		im_enable_interaction(); /* This element now tracks mouse inputs */

		if (i == 1) {
			TEST_ASSERT_FALSE(im_hovered());
			TEST_ASSERT_FALSE(im_pressed(IM_MOUSE_BUTTON_ANY, 0));
		}
		
		im_pop_frame();
		
		im_push_frame(frame_make(50, 50, 100, 100));
		
		im_enable_interaction();

		if (i == 1) {
			TEST_ASSERT_TRUE(im_hovered());
			TEST_ASSERT_TRUE(im_pressed(IM_MOUSE_BUTTON_ANY, 0));
		}
		
		/*
		Even if there's an additional element on top of the current one,
		unless we call `im_enable_interaction`, that element is not included in mouse detection.
		*/
		im_push_frame(IM_FILL);
		im_pop_frame();
		
		im_pop_frame();
		
		end();
	}
}

TEST(core_input, click_on_release) {
	for (int i = 0; i < 3; ++i) {
		begin();
		
		im_set_input_state(vec2_make(75, 75), i == 1 ? IM_MOUSE_BUTTON_LEFT : 0);
		
		im_push_frame(frame_make(0, 0, 100, 100));
		im_pop_frame();
		
		im_push_frame(frame_make(50, 50, 100, 100));
		
		im_enable_interaction();
		
		if (i == 2) {
			TEST_ASSERT_TRUE(im_clicked(IM_MOUSE_BUTTON_ANY, IM_CLICK_STARTS_INSIDE));
		}
		
		im_pop_frame();
		
		end();
	}
}

TEST(core_input, click_on_button_down) {
	for (int i = 0; i < 2; ++i) {
		begin();
		
		im_set_input_state(vec2_make(75, 75), i == 1 ? IM_MOUSE_BUTTON_LEFT : 0);
		
		im_push_frame(frame_make(0, 0, 100, 100));
		im_pop_frame();
		
		im_push_frame(frame_make(50, 50, 100, 100));
		
		im_enable_interaction();
		
		if (i == 1) {
			/* Click is detected as soon as mouse button is pressed */
			TEST_ASSERT_TRUE(im_clicked(IM_MOUSE_BUTTON_ANY, IM_CLICK_ON_BUTTON_DOWN));
		}
		
		im_pop_frame();
		
		end();
	}
}

TEST(core_input, click_starts_outside) {
	for (int i = 0; i < 3; ++i) {
		begin();
		
		/* Simulate mouse change over time */
		if (i == 0) {
			im_set_input_state(vec2_make(25, 25), 0);
		} else if (i == 1) {
			im_set_input_state(vec2_make(75, 75), IM_MOUSE_BUTTON_LEFT);
		} else if (i == 2) {
			im_set_input_state(vec2_make(75, 75), 0);
		}
		
		im_push_frame(frame_make(0, 0, 100, 100));
		im_pop_frame();
		
		im_push_frame(frame_make(50, 50, 100, 100));
		
		im_enable_interaction();
		
		if (i == 2) {
			/*
			Mouse was clicked when outside of this element, so even when moving
			the cursor over when releasing the button, click is not tracked.
			*/
			TEST_ASSERT_FALSE(im_clicked(IM_MOUSE_BUTTON_ANY, IM_CLICK_STARTS_INSIDE));
		}
		
		im_pop_frame();
		
		end();
	}
}

TEST(core_input, simple_drag) {
	for (int i = 0; i < 3; ++i) {
		begin();
		
		/* Simulate mouse change over time */
		if (i == 0) {
			im_set_input_state(vec2_make(25, 25), IM_MOUSE_BUTTON_LEFT);
			
			/* Taking exlusive ownership of the mouse. See `im_mouse_state_t` for more info */
			im_mouse_state()->locked = true;
			
			/* Drag is activated on mouse button down */
			TEST_ASSERT_TRUE(im_mouse_state()->drag.active);
			TEST_ASSERT_TRUE(im_mouse_state()->button_mask & IM_MOUSE_BUTTON_LEFT);
			TEST_ASSERT_EQUAL_VEC2(vec2_make(25, 25), im_mouse_state()->drag.start_position);
			TEST_ASSERT_EQUAL_VEC2(vec2_zero(), im_mouse_state()->drag.change);
		} else if (i == 1) {
			im_set_input_state(vec2_make(50, 50), IM_MOUSE_BUTTON_LEFT);
			
			TEST_ASSERT_EQUAL_VEC2(vec2_make(25, 25), im_mouse_state()->drag.change);
		} else if (i == 2) {
			im_set_input_state(vec2_make(75, 75), 0);
			
			TEST_ASSERT_FALSE(im_mouse_state()->drag.active);
		}
		
		
		im_push_frame(frame_make(0, 0, 100, 100));
		im_enable_interaction();
		
		if (i == 1) {
			/* Even though the mouse is over this element at this point, lock
			   prevents it from being detected */
			TEST_ASSERT_FALSE(im_hovered());
		}
		
		im_pop_frame();

		end();
	}
}

TEST(core_input, button_states) {
	im_set_input_state(vec2_zero(), IM_MOUSE_BUTTON_LEFT);
	
	TEST_ASSERT_TRUE(im_mouse_state()->button_mask & IM_MOUSE_BUTTON_LEFT);
	TEST_ASSERT_EQUAL(IM_MOUSE_BUTTON_LEFT, im_mouse_state()->last_button_down);
	TEST_ASSERT_EQUAL(0, im_mouse_state()->last_button_up);
	
	
	im_set_input_state(vec2_zero(), IM_MOUSE_BUTTON_LEFT | IM_MOUSE_BUTTON_RIGHT);
	
	TEST_ASSERT_TRUE(im_mouse_state()->button_mask & IM_MOUSE_BUTTON_ANY);
	TEST_ASSERT_EQUAL(IM_MOUSE_BUTTON_RIGHT, im_mouse_state()->last_button_down);
	TEST_ASSERT_EQUAL(0, im_mouse_state()->last_button_up);
	
	
	im_set_input_state(vec2_zero(), IM_MOUSE_BUTTON_RIGHT);
	
	TEST_ASSERT_TRUE(im_mouse_state()->button_mask & IM_MOUSE_BUTTON_RIGHT);
	TEST_ASSERT_EQUAL(IM_MOUSE_BUTTON_LEFT, im_mouse_state()->last_button_up);
	
	
	im_set_input_state(vec2_zero(), 0);
	
	TEST_ASSERT_EQUAL(0, im_mouse_state()->button_mask);
	TEST_ASSERT_EQUAL(IM_MOUSE_BUTTON_RIGHT, im_mouse_state()->last_button_up);
}

TEST_GROUP_RUNNER(core_input) {
	RUN_TEST_CASE(core_input, hover_and_press);
	RUN_TEST_CASE(core_input, overlapping_hover_and_press);
	RUN_TEST_CASE(core_input, click_on_release);
	RUN_TEST_CASE(core_input, click_on_button_down);
	RUN_TEST_CASE(core_input, click_starts_outside);
	RUN_TEST_CASE(core_input, simple_drag);
	RUN_TEST_CASE(core_input, button_states);
}
