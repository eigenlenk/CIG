#include "unity.h"
#include "fixture.h"
#include "cigcore.h"
#include "asserts.h"

TEST_GROUP(types);

TEST_SETUP(types) {}
TEST_TEAR_DOWN(types) {}

/* (( TEST CASES )) */

TEST(types, frame_constructors) {
	const cig_frame_t f0 = cig_frame_zero();
	const cig_frame_t f1 = cig_frame_make(1, 2, 3, 4);
	
	TEST_ASSERT(f0.x == 0 && f0.y == 0 && f0.w == 0 && f0.h == 0);
	TEST_ASSERT(f1.x == 1 && f1.y == 2 && f1.w == 3 && f1.h == 4);
}

TEST(types, frame_comparator) {
	const cig_frame_t a = cig_frame_make(10, 10, 100, 100);
	const cig_frame_t b = cig_frame_make(10, 10, 100, 100);
	const cig_frame_t c = cig_frame_make(10, 10, 90, 100);
	
	TEST_ASSERT_TRUE(cig_frame_cmp(a, b));
	TEST_ASSERT_FALSE(cig_frame_cmp(b, c));
}

TEST(types, frame_point_check) {
	const cig_frame_t f = cig_frame_make(40, 40, 50, 30);
	
	TEST_ASSERT_TRUE(cig_frame_contains(f, cig_vec2_make(50, 50)));
	TEST_ASSERT_FALSE(cig_frame_contains(f, cig_vec2_make(30, 50)));
}

TEST(types, frame_offset) {
	const cig_frame_t f = cig_frame_offset(cig_frame_make(40, 40, 50, 30), 10, 10);
	
	TEST_ASSERT(f.x == 50 && f.y == 50);
}

TEST(types, frame_inset) {
	const cig_frame_t f = cig_frame_inset(cig_frame_make(40, 40, 100, 100), insets_make(10, 20, 5, 15));
	
	TEST_ASSERT(cig_frame_cmp(f, cig_frame_make(50, 60, 85, 65)));
}

TEST(types, frame_contains_relative_frame) {
	const cig_frame_t f = cig_frame_make(40, 40, 50, 30);
	
	TEST_ASSERT_TRUE(cig_frame_wholly_contains_relative_frame(
		cig_frame_make(40, 40, 50, 30),
		cig_frame_make(10, 10, 20, 20)
	));
	TEST_ASSERT_FALSE(cig_frame_wholly_contains_relative_frame(
		cig_frame_make(40, 40, 50, 30),
		cig_frame_make(10, 10, 20, 30)
	));
}

TEST(types, frame_intersections) {
	TEST_ASSERT_TRUE(cig_frame_intersects(cig_frame_make(50, 50, 100, 100), cig_frame_make(40, 40, 100, 100)));
	TEST_ASSERT_FALSE(cig_frame_intersects(cig_frame_make(50, 50, 100, 100), cig_frame_make(0, 0, 50, 50)));
}

TEST(types, frame_center) {
	TEST_ASSERT_EQUAL_VEC2(cig_vec2_make(50, 50), cig_frame_center(cig_frame_make(0, 0, 100, 100)));
}

TEST(types, frame_containing) {
	TEST_ASSERT_EQUAL_FRAME(
		cig_frame_make(0, 0, 60, 60),
		cig_frame_containing(
			cig_frame_make(0, 0, 30, 30),
			cig_frame_make(50, 20, 10, 40)
		)
	);
}

TEST(types, frame_union) {
	TEST_ASSERT_EQUAL_FRAME(
		cig_frame_make(40, 20, 10, 10),
		cig_frame_union(
			cig_frame_make(0, 0, 50, 30),
			cig_frame_make(40, 20, 10, 40)
		)
	);
	
	TEST_ASSERT_EQUAL_FRAME(
		cig_frame_make(0, 0, 0, 0),
		cig_frame_union(
			cig_frame_make(0, 0, 50, 30),
			cig_frame_make(40, 40, 10, 40)
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
	const cig_vec2_t v0 = cig_vec2_zero();
	const cig_vec2_t v1 = cig_vec2_make(1, 2);
	const cig_vec2_t v2 = cig_vec2_invalid();
	
	TEST_ASSERT(v0.x == 0 && v0.y == 0);
	TEST_ASSERT(v1.x == 1 && v1.y == 2);
	TEST_ASSERT_TRUE(cig_vec2_valid(v0));
	TEST_ASSERT_TRUE(cig_vec2_valid(v1));
	TEST_ASSERT_FALSE(cig_vec2_valid(v2));
}

TEST(types, vec2_operations) {
	TEST_ASSERT_EQUAL_VEC2(cig_vec2_make(5, 7), cig_vec2_add(cig_vec2_make(3, 4), cig_vec2_make(2, 3)));
	TEST_ASSERT_EQUAL_VEC2(cig_vec2_make(8, 0), cig_vec2_sub(cig_vec2_make(9, 5), cig_vec2_make(1, 5)));
	TEST_ASSERT_EQUAL_VEC2(cig_vec2_make(6, 10), cig_vec2_mul(cig_vec2_make(3, 5), 2));
	TEST_ASSERT_EQUAL_VEC2(cig_vec2_make(8, 4), cig_vec2_div(cig_vec2_make(16, 8), 2));
}

TEST(types, vec2_comparator) {
	const cig_vec2_t a = cig_vec2_make(10, 10);
	const cig_vec2_t b = cig_vec2_make(10, 10);
	const cig_vec2_t c = cig_vec2_make(20, 20);
	
	TEST_ASSERT_TRUE(cig_vec2_cmp(a, b));
	TEST_ASSERT_FALSE(cig_vec2_cmp(b, c));
}

TEST(types, vec2_math_utils) {
	TEST_ASSERT_GREATER_THAN(0, cig_vec2_sign(cig_vec2_make(10, 14), cig_vec2_make(2, 3), cig_vec2_make(5, 7)));
	TEST_ASSERT_LESS_THAN(0, cig_vec2_sign(cig_vec2_make(4, 2), cig_vec2_make(1, 3), cig_vec2_make(5, 7)));
	TEST_ASSERT_EQUAL_INT(0, cig_vec2_sign(cig_vec2_make(5, 2), cig_vec2_make(0, 2), cig_vec2_make(10, 2)));
	TEST_ASSERT_DOUBLE_WITHIN(0.001, 2.828, cig_vec2_dist(cig_vec2_make(1, 1), cig_vec2_make(3, 3)));
	TEST_ASSERT_EQUAL_INT(32, cig_vec2_distsq(cig_vec2_make(1, 1), cig_vec2_make(5, 5)));
	TEST_ASSERT_EQUAL_VEC2(cig_vec2_make(5, 5), cig_vec2_abs(cig_vec2_make(-5, -5)));
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
