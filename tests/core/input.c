#include "unity.h"
#include "fixture.h"
#include "imgui.h"
#include "asserts.h"

#define ITERS 2
#define ASSERT_ITER 1

TEST_GROUP(core_input);

TEST_SETUP(core_input) {}
TEST_TEAR_DOWN(core_input) {}

static void begin() {
	im_begin_layout(NULL, frame_make(0, 0, 640, 480));
}

static void end() {
	im_end_layout();
}

TEST(core_input, mouse_hover_and_press) {
	register int i;
	for (i = 0; i < ITERS; ++i) {
		begin();
		
		im_set_input_state(vec2_make(50, 50), i == ASSERT_ITER ? IM_MOUSE_BUTTON_LEFT : 0);
		
		im_push_frame(frame_make(0, 0, 100, 100));
		
		if (i == ASSERT_ITER) {
			TEST_ASSERT_TRUE(im_hovered());
			TEST_ASSERT_TRUE(im_pressed(IM_MOUSE_BUTTON_ANY, IM_MOUSE_PRESS_INSIDE));
		}
		
		im_pop_frame();
		
		end();
	}
}

TEST(core_input, mouse_overlap_hover_and_press) {
	register int i;
	for (i = 0; i < ITERS; ++i) {
		begin();
		
		im_set_input_state(vec2_make(75, 75), i == ASSERT_ITER ? IM_MOUSE_BUTTON_LEFT : 0);
		
		im_push_frame(frame_make(0, 0, 100, 100));
		
		if (i == ASSERT_ITER) {
			TEST_ASSERT_FALSE(im_hovered());
			TEST_ASSERT_FALSE(im_pressed(IM_MOUSE_BUTTON_ANY, 0));
		}
		
		im_pop_frame();
		
		im_push_frame(frame_make(50, 50, 100, 100));
		
		if (i == ASSERT_ITER) {
			TEST_ASSERT_TRUE(im_hovered());
			TEST_ASSERT_TRUE(im_pressed(IM_MOUSE_BUTTON_ANY, 0));
		}
		
		im_pop_frame();
		
		end();
	}
}


TEST_GROUP_RUNNER(core_input) {
	RUN_TEST_CASE(core_input, mouse_hover_and_press);
	RUN_TEST_CASE(core_input, mouse_overlap_hover_and_press);
}
