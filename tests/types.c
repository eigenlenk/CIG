#include "unity.h"
#include "fixture.h"
#include "cigcore.h"
#include "asserts.h"

/* Declare a stack type */
#define STACK_CAPACITY_int 8
DECLARE_ARRAY_STACK_T(int);

TEST_GROUP(types);

TEST_SETUP(types) {}
TEST_TEAR_DOWN(types) {}

/* (( TEST CASES )) */

TEST(types, rect_constructors) {
	const cig_rect_t r0 = cig_rect_zero();
	const cig_rect_t r1 = cig_rect_make(1, 2, 3, 4);
	
	TEST_ASSERT(r0.x == 0 && r0.y == 0 && r0.w == 0 && r0.h == 0);
	TEST_ASSERT(r1.x == 1 && r1.y == 2 && r1.w == 3 && r1.h == 4);
}

TEST(types, rect_comparator) {
	const cig_rect_t a = cig_rect_make(10, 10, 100, 100);
	const cig_rect_t b = cig_rect_make(10, 10, 100, 100);
	const cig_rect_t c = cig_rect_make(10, 10, 90, 100);
	
	TEST_ASSERT_TRUE(cig_rect_cmp(a, b));
	TEST_ASSERT_FALSE(cig_rect_cmp(b, c));
}

TEST(types, rect_point_check) {
	const cig_rect_t r = cig_rect_make(40, 40, 50, 30);
	
	TEST_ASSERT_TRUE(cig_rect_contains(r, cig_vec2_make(50, 50)));
	TEST_ASSERT_FALSE(cig_rect_contains(r, cig_vec2_make(30, 50)));
}

TEST(types, rect_offset) {
	const cig_rect_t r = cig_rect_offset(cig_rect_make(40, 40, 50, 30), 10, 10);
	
	TEST_ASSERT(r.x == 50 && r.y == 50);
}

TEST(types, rect_inset) {
	const cig_rect_t r = cig_rect_inset(cig_rect_make(40, 40, 100, 100), cig_insets_make(10, 20, 5, 15));
	
	TEST_ASSERT(cig_rect_cmp(r, cig_rect_make(50, 60, 85, 65)));
}

TEST(types, rect_intersections) {
	TEST_ASSERT_TRUE(cig_rect_intersects(cig_rect_make(50, 50, 100, 100), cig_rect_make(40, 40, 100, 100)));
	TEST_ASSERT_FALSE(cig_rect_intersects(cig_rect_make(50, 50, 100, 100), cig_rect_make(0, 0, 50, 50)));
}

TEST(types, rect_center) {
	TEST_ASSERT_EQUAL_VEC2(cig_vec2_make(50, 50), cig_rect_center(cig_rect_make(0, 0, 100, 100)));
}

TEST(types, rect_containing) {
	TEST_ASSERT_EQUAL_RECT(
		cig_rect_make(0, 0, 60, 60),
		cig_rect_containing(
			cig_rect_make(0, 0, 30, 30),
			cig_rect_make(50, 20, 10, 40)
		)
	);
}

TEST(types, rect_union) {
	TEST_ASSERT_EQUAL_RECT(
		cig_rect_make(40, 20, 10, 10),
		cig_rect_union(
			cig_rect_make(0, 0, 50, 30),
			cig_rect_make(40, 20, 10, 40)
		)
	);
	
	TEST_ASSERT_EQUAL_RECT(
		cig_rect_make(0, 0, 0, 0),
		cig_rect_union(
			cig_rect_make(0, 0, 50, 30),
			cig_rect_make(40, 40, 10, 40)
		)
	);
}

TEST(types, insets_constructors) {
	const cig_insets_t i0 = cig_insets_zero();
	const cig_insets_t i1 = cig_insets_make(10, 20, 30, 40);
	const cig_insets_t i2 = cig_insets_uniform(5);
	const cig_insets_t i3 = cig_insets_horizontal(8);
	const cig_insets_t i4 = cig_insets_vertical(1);
	
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

TEST(types, stack_operations) {
  int_stack_t si = INIT_STACK(int);
  
  TEST_ASSERT_EQUAL_INT(8, si.capacity);
  TEST_ASSERT_EQUAL_INT(0, si.size);
  
  si.push(&si, 5);
  
  TEST_ASSERT_EQUAL_INT(1, si.size);
  
  /* stack_int_peek == si.peek */
  TEST_ASSERT_EQUAL_INT(5, stack_int_peek(&si, 0));
  
  si.pop(&si);
  
  TEST_ASSERT_EQUAL_INT(0, si.size);
}

TEST_GROUP_RUNNER(types) {
  RUN_TEST_CASE(types, rect_constructors);
  RUN_TEST_CASE(types, rect_comparator);
  RUN_TEST_CASE(types, rect_point_check);
  RUN_TEST_CASE(types, rect_offset);
  RUN_TEST_CASE(types, rect_inset);
  RUN_TEST_CASE(types, rect_intersections);
  RUN_TEST_CASE(types, rect_center);
  RUN_TEST_CASE(types, rect_containing);
  RUN_TEST_CASE(types, rect_union);
	RUN_TEST_CASE(types, insets_constructors);
	RUN_TEST_CASE(types, vec2_constructors_and_validity);
	RUN_TEST_CASE(types, vec2_operations);
	RUN_TEST_CASE(types, vec2_comparator);
	RUN_TEST_CASE(types, vec2_math_utils);
	RUN_TEST_CASE(types, stack_operations);
}
