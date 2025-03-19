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
		
		im_hovered(); /* This needs to be called on every frame */
		
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
		
		im_hovered(); /* This needs to be called on every frame */
		
		if (i == 1) {
			TEST_ASSERT_FALSE(im_hovered());
			TEST_ASSERT_FALSE(im_pressed(IM_MOUSE_BUTTON_ANY, 0));
		}
		
		im_pop_frame();
		
		im_push_frame(frame_make(50, 50, 100, 100));
		
		im_hovered();	/* This needs to be called on every frame */
		
		if (i == 1) {
			TEST_ASSERT_TRUE(im_hovered());
			TEST_ASSERT_TRUE(im_pressed(IM_MOUSE_BUTTON_ANY, 0));
		}
		
		/*
		Even if there's an additional element on top of the current one,
		unless we call `im_hovered`, that element is not included in mouse detection.
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
		im_hovered();
		im_pop_frame();
		
		im_push_frame(frame_make(50, 50, 100, 100));
		im_hovered();
		
		if (i == 2) {
			TEST_ASSERT_TRUE(im_clicked(IM_MOUSE_BUTTON_ANY, 0));
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
		im_hovered();
		im_pop_frame();
		
		im_push_frame(frame_make(50, 50, 100, 100));
		im_hovered();
		
		if (i == 1) {
			/* Click is detected as soon as mouse button is pressed */
			TEST_ASSERT_TRUE(im_clicked(IM_MOUSE_BUTTON_ANY, IM_CLICK_ON_BUTTON_DOWN));
		}
		
		im_pop_frame();
		
		end();
	}
}

TEST_GROUP_RUNNER(core_input) {
	RUN_TEST_CASE(core_input, hover_and_press);
	RUN_TEST_CASE(core_input, overlapping_hover_and_press);
	RUN_TEST_CASE(core_input, click_on_release);
	RUN_TEST_CASE(core_input, click_on_button_down);
}
