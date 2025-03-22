#ifndef CIM_CUSTOM_TYPE_ASSERTS_H
#define CIM_CUSTOM_TYPE_ASSERTS_H

#include "unity.h"

static void assert_frame_equal(const cig_frame_t exp, const cig_frame_t act, unsigned int line) {
	char message[64];
	sprintf(message, "(%d, %d, %d, %d) != (%d, %d, %d, %d)", exp.x, exp.y, exp.w, exp.h, act.x, act.y, act.w, act.h);
	UNITY_TEST_ASSERT(cig_frame_cmp(exp, act), line, message);
}

#define TEST_ASSERT_EQUAL_FRAME(expected, actual) assert_frame_equal(expected, actual, __LINE__)

static void assert_vec2_equal(const cig_vec2_t exp, const cig_vec2_t act, unsigned int line) {
	char message[48];
	sprintf(message, "(%d, %d) != (%d, %d)", exp.x, exp.y, act.x, act.y);
	UNITY_TEST_ASSERT(cig_vec2_cmp(exp, act), line, message);
}

#define TEST_ASSERT_EQUAL_VEC2(expected, actual) assert_vec2_equal(expected, actual, __LINE__)

#endif // CIM_CUSTOM_TYPE_ASSERTS_H