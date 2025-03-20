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
	const insets_t i0 = insets_zero();
	const insets_t i1 = insets_make(10, 20, 30, 40);
	const insets_t i2 = insets_uniform(5);
	const insets_t i3 = insets_horizontal(8);
	const insets_t i4 = insets_vertical(1);
	
	TEST_ASSERT(i0.left == 0  && i0.top == 0  && i0.right == 0  && i0.bottom == 0 );
	TEST_ASSERT(i1.left == 10 && i1.top == 20 && i1.right == 30 && i1.bottom == 40);
	TEST_ASSERT(i2.left == 5  && i2.top == 5  && i2.right == 5  && i2.bottom == 5 );
	TEST_ASSERT(i3.left == 8  && i3.top == 0  && i3.right == 8  && i3.bottom == 0 );
	TEST_ASSERT(i4.left == 0  && i4.top == 1  && i4.right == 0  && i4.bottom == 1 );
}

TEST(types, vec2_constructors_and_validity) {
	const vec2 v0 = vec2_zero();
	const vec2 v1 = vec2_make(1, 2);
	const vec2 v2 = vec2_invalid();
	
	TEST_ASSERT(v0.x == 0 && v0.y == 0);
	TEST_ASSERT(v1.x == 1 && v1.y == 2);
	TEST_ASSERT_TRUE(vec2_valid(v0));
	TEST_ASSERT_TRUE(vec2_valid(v1));
	TEST_ASSERT_FALSE(vec2_valid(v2));
}

TEST(types, vec2_operations) {
	TEST_ASSERT_EQUAL_VEC2(vec2_make(5, 7), vec2_add(vec2_make(3, 4), vec2_make(2, 3)));
	TEST_ASSERT_EQUAL_VEC2(vec2_make(8, 0), vec2_sub(vec2_make(9, 5), vec2_make(1, 5)));
	TEST_ASSERT_EQUAL_VEC2(vec2_make(6, 10), vec2_mul(vec2_make(3, 5), 2));
	TEST_ASSERT_EQUAL_VEC2(vec2_make(8, 4), vec2_div(vec2_make(16, 8), 2));
}

TEST(types, vec2_comparator) {
	const vec2 a = vec2_make(10, 10);
	const vec2 b = vec2_make(10, 10);
	const vec2 c = vec2_make(20, 20);
	
	TEST_ASSERT_TRUE(vec2_cmp(a, b));
	TEST_ASSERT_FALSE(vec2_cmp(b, c));
}

TEST(types, vec2_math_utils) {
	TEST_ASSERT_GREATER_THAN(0, vec2_sign(vec2_make(10, 14), vec2_make(2, 3), vec2_make(5, 7)));
	TEST_ASSERT_LESS_THAN(0, vec2_sign(vec2_make(4, 2), vec2_make(1, 3), vec2_make(5, 7)));
	TEST_ASSERT_EQUAL_INT(0, vec2_sign(vec2_make(5, 2), vec2_make(0, 2), vec2_make(10, 2)));
	TEST_ASSERT_DOUBLE_WITHIN(0.001, 2.828, vec2_dist(vec2_make(1, 1), vec2_make(3, 3)));
	TEST_ASSERT_EQUAL_INT(32, vec2_distsq(vec2_make(1, 1), vec2_make(5, 5)));
	TEST_ASSERT_EQUAL_VEC2(vec2_make(5, 5), vec2_abs(vec2_make(-5, -5)));
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
	RUN_TEST_CASE(types, vec2_constructors_and_validity);
	RUN_TEST_CASE(types, vec2_operations);
	RUN_TEST_CASE(types, vec2_comparator);
	RUN_TEST_CASE(types, vec2_math_utils);
}
