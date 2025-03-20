#include "unity.h"
#include "fixture.h"
#include "imgui.h"
#include "asserts.h"

TEST_GROUP(types);

TEST_SETUP(types) {}
TEST_TEAR_DOWN(types) {}

/* (( TEST CASES )) */

TEST(types, frame_constructors) {
	const frame_t f0 = frame_zero();
	const frame_t f1 = frame_make(1, 2, 3, 4);
	
	TEST_ASSERT(f0.x == 0 && f0.y == 0 && f0.w == 0 && f0.h == 0);
	TEST_ASSERT(f1.x == 1 && f1.y == 2 && f1.w == 3 && f1.h == 4);
}

TEST(types, frame_comparator) {
	const frame_t a = frame_make(10, 10, 100, 100);
	const frame_t b = frame_make(10, 10, 100, 100);
	const frame_t c = frame_make(10, 10, 90, 100);
	
	TEST_ASSERT_TRUE(frame_cmp(a, b));
	TEST_ASSERT_FALSE(frame_cmp(b, c));
}

TEST(types, frame_point_check) {
	const frame_t f = frame_make(40, 40, 50, 30);
	
	TEST_ASSERT_TRUE(frame_contains(f, VEC2(50, 50)));
	TEST_ASSERT_FALSE(frame_contains(f, VEC2(30, 50)));
}

TEST(types, frame_offset) {
	const frame_t f = frame_offset(frame_make(40, 40, 50, 30), 10, 10);
	
	TEST_ASSERT(f.x == 50 && f.y == 50);
}

TEST(types, frame_inset) {
	const frame_t f = frame_inset(frame_make(40, 40, 100, 100), insets_make(10, 20, 5, 15));
	
	TEST_ASSERT(frame_cmp(f, frame_make(50, 60, 85, 65)));
}

TEST(types, frame_contains_relative_frame) {
	const frame_t f = frame_make(40, 40, 50, 30);
	
	TEST_ASSERT_TRUE(frame_wholly_contains_relative_frame(
		frame_make(40, 40, 50, 30),
		frame_make(10, 10, 20, 20)
	));
	TEST_ASSERT_FALSE(frame_wholly_contains_relative_frame(
		frame_make(40, 40, 50, 30),
		frame_make(10, 10, 20, 30)
	));
}

TEST(types, frame_intersections) {
	TEST_ASSERT_TRUE(frame_intersects(frame_make(50, 50, 100, 100), frame_make(40, 40, 100, 100)));
	TEST_ASSERT_FALSE(frame_intersects(frame_make(50, 50, 100, 100), frame_make(0, 0, 50, 50)));
}

TEST(types, frame_center) {
	TEST_ASSERT_EQUAL_VEC2(vec2_make(50, 50), frame_center(frame_make(0, 0, 100, 100)));
}

TEST(types, frame_containing) {
	TEST_ASSERT_EQUAL_FRAME(
		frame_make(0, 0, 60, 60),
		frame_containing(
			frame_make(0, 0, 30, 30),
			frame_make(50, 20, 10, 40)
		)
	);
}

TEST(types, frame_union) {
	TEST_ASSERT_EQUAL_FRAME(
		frame_make(40, 20, 10, 10),
		frame_union(
			frame_make(0, 0, 50, 30),
			frame_make(40, 20, 10, 40)
		)
	);
	
	TEST_ASSERT_EQUAL_FRAME(
		frame_make(0, 0, 0, 0),
		frame_union(
			frame_make(0, 0, 50, 30),
			frame_make(40, 40, 10, 40)
		)
	);
}

TEST(types, insets_constructors) {
	TEST_ASSERT_EQUAL_INSETS(10, 20, 30, 40, insets_make(10, 20, 30, 40));
	TEST_ASSERT_EQUAL_INSETS(0, 0, 0, 0, insets_zero());
	TEST_ASSERT_EQUAL_INSETS(5, 5, 5, 5, insets_uniform(5));
	TEST_ASSERT_EQUAL_INSETS(8, 0, 8, 0, insets_horizontal(8));
	TEST_ASSERT_EQUAL_INSETS(0, 1, 0, 1, insets_vertical(1));
}

TEST_GROUP_RUNNER(types) {
  RUN_TEST_CASE(types, frame_constructors);
  RUN_TEST_CASE(types, frame_comparator);
  RUN_TEST_CASE(types, frame_point_check);
  RUN_TEST_CASE(types, frame_offset);
  RUN_TEST_CASE(types, frame_inset);
  RUN_TEST_CASE(types, frame_contains_relative_frame);
  RUN_TEST_CASE(types, frame_intersections);
  RUN_TEST_CASE(types, frame_center);
  RUN_TEST_CASE(types, frame_containing);
  RUN_TEST_CASE(types, frame_union);
	RUN_TEST_CASE(types, insets_constructors);
}
