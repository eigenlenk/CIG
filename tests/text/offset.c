#include "unity.h"
#include "fixture.h"
#include "cigtext.h"
#include "utf8.h"

TEST_GROUP(text_offset);

TEST_SETUP(text_offset)
{
}

TEST_TEAR_DOWN(text_offset)
{
}

/* ┌────────────┐
   │ TEST CASES │
   └────────────┘ */

TEST(text_offset, find_line_start)
{
  /*                |0     1 2    3      4 5 6 */
  const char *str = "Zero\n\nTwo\nThree\n\n\nSix";
  utf8_string ustr = make_utf8_string(str);

  // Move forward
  TEST_ASSERT_EQUAL_PTR(str+5, cig_utf8_string_line_location(ustr, str, 1));
  TEST_ASSERT_EQUAL_PTR(str+16, cig_utf8_string_line_location(ustr, str, 4));
  TEST_ASSERT_EQUAL_PTR(str+18, cig_utf8_string_line_location(ustr, str, 6));

  // Move backward
  TEST_ASSERT_EQUAL_PTR(str+10, cig_utf8_string_line_location(ustr, str + 16, -1)); // Starting on line 4

  // Not going out of bounds
  TEST_ASSERT_EQUAL_PTR(str, cig_utf8_string_line_location(ustr, str, -1));
  TEST_ASSERT_EQUAL_PTR(str+18, cig_utf8_string_line_location(ustr, str, 7));
  TEST_ASSERT_EQUAL_PTR(str+18, cig_utf8_string_line_location(ustr, str, 99));
}

TEST_GROUP_RUNNER(text_offset)
{
  RUN_TEST_CASE(text_offset, find_line_start);
}
