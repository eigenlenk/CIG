#include "unity.h"
#include "fixture.h"
#include "cigcore.h"
#include "asserts.h"

TEST_GROUP(core_input);

static cig_context_t ctx = { 0 };

TEST_SETUP(core_input) {}
TEST_TEAR_DOWN(core_input) {}

static void begin() {
	cig_begin_layout(&ctx, NULL, cig_rect_make(0, 0, 640, 480));
}

static void end() {
	cig_end_layout();
}

TEST(core_input, hover_and_press) {
	for (int i = 0; i < 2; ++i) {
		begin();
		
		cig_set_input_state(cig_vec2_make(50, 50), i == 1 ? CIG_INPUT_MOUSE_BUTTON_LEFT : 0);
		
		cig_push_frame(cig_rect_make(0, 0, 100, 100));
		
		cig_enable_interaction(); /* This element now tracks mouse inputs */
		
		if (i == 1) {
			TEST_ASSERT_TRUE(cig_hovered());
			TEST_ASSERT_TRUE(cig_pressed(CIG_MOUSE_BUTTON_ANY, CIG_PRESS_INSIDE));
		}
		
		cig_pop_frame();
		
		end();
	}
}

TEST(core_input, overlapping_hover_and_press) {
	for (int i = 0; i < 2; ++i) {
		begin();
		
		cig_set_input_state(cig_vec2_make(75, 75), i == 1 ? CIG_INPUT_MOUSE_BUTTON_LEFT : 0);
		
		cig_push_frame(cig_rect_make(0, 0, 100, 100));
		
		cig_enable_interaction(); /* This element now tracks mouse inputs */

		if (i == 1) {
			TEST_ASSERT_FALSE(cig_hovered());
			TEST_ASSERT_FALSE(cig_pressed(CIG_MOUSE_BUTTON_ANY, 0));
		}
		
		cig_pop_frame();
		
		cig_push_frame(cig_rect_make(50, 50, 100, 100));
		
		cig_enable_interaction();

		if (i == 1) {
			TEST_ASSERT_TRUE(cig_hovered());
			TEST_ASSERT_TRUE(cig_pressed(CIG_MOUSE_BUTTON_ANY, 0));
		}
		
		/*
		Even if there's an additional element on top of the current one,
		unless we call `im_enable_interaction`, that element is not included in mouse detection.
		*/
		cig_push_frame(CIG_FILL);
		cig_pop_frame();
		
		cig_pop_frame();
		
		end();
	}
}

TEST(core_input, click_on_release) {
	for (int i = 0; i < 3; ++i) {
		begin();
		
		cig_set_input_state(cig_vec2_make(75, 75), i == 1 ? CIG_INPUT_MOUSE_BUTTON_LEFT : 0);
		
		cig_push_frame(cig_rect_make(0, 0, 100, 100));
		cig_pop_frame();
		
		cig_push_frame(cig_rect_make(50, 50, 100, 100));
		
		cig_enable_interaction();
		
		if (i == 2) {
			TEST_ASSERT_TRUE(cig_clicked(CIG_MOUSE_BUTTON_ANY, CIG_CLICK_STARTS_INSIDE));
		}
		
		cig_pop_frame();
		
		end();
	}
}

TEST(core_input, click_on_button_down) {
	for (int i = 0; i < 2; ++i) {
		begin();
		
		cig_set_input_state(cig_vec2_make(75, 75), i == 1 ? CIG_INPUT_MOUSE_BUTTON_LEFT : 0);
		
		cig_push_frame(cig_rect_make(0, 0, 100, 100));
		cig_pop_frame();
		
		cig_push_frame(cig_rect_make(50, 50, 100, 100));
		
		cig_enable_interaction();
		
		if (i == 1) {
			/* Click is detected as soon as mouse button is pressed */
			TEST_ASSERT_TRUE(cig_clicked(CIG_MOUSE_BUTTON_ANY, CIG_CLICK_ON_PRESS));
		}
		
		cig_pop_frame();
		
		end();
	}
}

TEST(core_input, click_starts_outside) {
	for (int i = 0; i < 3; ++i) {
		begin();
		
		/* Simulate mouse change over time */
		if (i == 0) {
			cig_set_input_state(cig_vec2_make(25, 25), 0);
		} else if (i == 1) {
			cig_set_input_state(cig_vec2_make(75, 75), CIG_INPUT_MOUSE_BUTTON_LEFT);
		} else if (i == 2) {
			cig_set_input_state(cig_vec2_make(75, 75), 0);
		}
		
		cig_push_frame(cig_rect_make(0, 0, 100, 100));
		cig_pop_frame();
		
		cig_push_frame(cig_rect_make(50, 50, 100, 100));
		
		cig_enable_interaction();
		
		if (i == 2) {
			/*
			Mouse was clicked when outside of this element, so even when moving
			the cursor over when releasing the button, click is not tracked.
			*/
			TEST_ASSERT_FALSE(cig_clicked(CIG_MOUSE_BUTTON_ANY, CIG_CLICK_STARTS_INSIDE));
		}
		
		cig_pop_frame();
		
		end();
	}
}

TEST(core_input, simple_drag) {
	for (int i = 0; i < 3; ++i) {
		begin();
		
		/* Simulate mouse change over time */
		if (i == 0) {
			cig_set_input_state(cig_vec2_make(25, 25), CIG_INPUT_MOUSE_BUTTON_LEFT);
			
			/* Taking exlusive ownership of the mouse. See `cig_mouse_state_t` for more info */
			cig_input_state()->locked = true;
			
			/* Drag is activated on mouse button down */
			TEST_ASSERT_TRUE(cig_input_state()->drag.active);
			TEST_ASSERT_TRUE(cig_input_state()->action_mask & CIG_INPUT_MOUSE_BUTTON_LEFT);
			TEST_ASSERT_EQUAL_VEC2(cig_vec2_make(25, 25), cig_input_state()->drag.start_position);
			TEST_ASSERT_EQUAL_VEC2(cig_vec2_zero(), cig_input_state()->drag.change);
		} else if (i == 1) {
			cig_set_input_state(cig_vec2_make(50, 50), CIG_INPUT_MOUSE_BUTTON_LEFT);
			
			TEST_ASSERT_EQUAL_VEC2(cig_vec2_make(25, 25), cig_input_state()->drag.change);
		} else if (i == 2) {
			cig_set_input_state(cig_vec2_make(75, 75), 0);
			
			TEST_ASSERT_FALSE(cig_input_state()->drag.active);
		}
		
		
		cig_push_frame(cig_rect_make(0, 0, 100, 100));
		cig_enable_interaction();
		
		if (i == 1) {
			/* Even though the mouse is over this element at this point, lock
			   prevents it from being detected */
			TEST_ASSERT_FALSE(cig_hovered());
		}
		
		cig_pop_frame();

		end();
	}
}

TEST(core_input, button_states) {
	/* (Time 0) */
	cig_set_input_state(cig_vec2_zero(), CIG_INPUT_MOUSE_BUTTON_LEFT);
	
	TEST_ASSERT_TRUE(cig_input_state()->action_mask & CIG_INPUT_MOUSE_BUTTON_LEFT);
	TEST_ASSERT_EQUAL(CIG_INPUT_MOUSE_BUTTON_LEFT, cig_input_state()->last_action_began);
	TEST_ASSERT_EQUAL(0, cig_input_state()->last_action_ended);
	TEST_ASSERT_EQUAL(BEGAN, cig_input_state()->click_state);
	
	/* (T1) */
	cig_set_input_state(cig_vec2_zero(), CIG_INPUT_MOUSE_BUTTON_LEFT | CIG_INPUT_MOUSE_BUTTON_RIGHT);
	
	TEST_ASSERT_TRUE(cig_input_state()->action_mask & CIG_MOUSE_BUTTON_ANY);
	TEST_ASSERT_EQUAL(CIG_INPUT_MOUSE_BUTTON_RIGHT, cig_input_state()->last_action_began);
	TEST_ASSERT_EQUAL(0, cig_input_state()->last_action_ended);
	/* Maybe this should report a failed case or something because you're not
	   pressing down both mouse buttons and expecting a click event normally? */
	TEST_ASSERT_EQUAL(NEITHER, cig_input_state()->click_state);
	
	/* (T2) */
	cig_set_input_state(cig_vec2_zero(), CIG_INPUT_MOUSE_BUTTON_RIGHT);
	
	TEST_ASSERT_TRUE(cig_input_state()->action_mask & CIG_INPUT_MOUSE_BUTTON_RIGHT);
	TEST_ASSERT_EQUAL(CIG_INPUT_MOUSE_BUTTON_LEFT, cig_input_state()->last_action_ended);
	TEST_ASSERT_EQUAL(NEITHER, cig_input_state()->click_state);
	
	/* (T3) */
	cig_set_input_state(cig_vec2_zero(), 0);
	
	TEST_ASSERT_EQUAL(0, cig_input_state()->action_mask);
	TEST_ASSERT_EQUAL(CIG_INPUT_MOUSE_BUTTON_RIGHT, cig_input_state()->last_action_ended);
	TEST_ASSERT_EQUAL(ENDED, cig_input_state()->click_state);
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
