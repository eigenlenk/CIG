#include "unity.h"
#include "fixture.h"
#include "cigcore.h"
#include "cigcorem.h"
#include "asserts.h"

TEST_GROUP(core_macros);

static cig_context_t ctx = { 0 };

TEST_SETUP(core_macros) {
  cig_init_context(&ctx);
  cig_begin_layout(&ctx, NULL, cig_r_make(0, 0, 640, 480), 0.1f);
}

TEST_TEAR_DOWN(core_macros) {
  cig_end_layout();
}

/*  ┌────────────┐
    │ TEST CASES │
    └────────────┘ */

TEST(core_macros, cig) {
  /*  This creates a 500x400 frame with 4 unit inset and centers it in the root frame */
  CIG(RECT_CENTERED(500, 400), CIG_INSETS(cig_i_uniform(4))) {
    TEST_ASSERT_EQUAL_RECT(cig_r_make(70, 40, 500, 400), cig_rect());

    /*  Regular control flow works in here */
    for (int i = 0; i < 2; ++i) { }

    CIG(_) { /* _ stands for RECT_AUTO */
      TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 0, 492, 392), cig_rect());
    }
  }
}

TEST(core_macros, vstack) {
  CIG_VSTACK(
    RECT_AUTO,
    CIG_PARAMS({
      CIG_HEIGHT(200),
      CIG_SPACING(10),
      CIG_LIMIT_VERTICAL(2)
    })
  ) {
    for (int i = 0; i < 2; ++i) {
      CIG(_) {
        TEST_ASSERT_EQUAL_RECT(cig_r_make(0, i*(200+10), 640, 200), cig_rect());
      }
    }
  }
}

TEST(core_macros, hstack) {
  CIG_HSTACK(
    RECT_AUTO,
    CIG_PARAMS({
      CIG_WIDTH(200),
      CIG_SPACING(10),
      CIG_LIMIT_HORIZONTAL(2)
    })
  ) {
    for (int i = 0; i < 2; ++i) {
      CIG(_) {
        TEST_ASSERT_EQUAL_RECT(cig_r_make(i*(200+10), 0, 200, 480), cig_rect());
      }
    }
  }
}

TEST(core_macros, grid) {
  CIG_GRID(
    RECT_AUTO,
    CIG_PARAMS({
      CIG_COLUMNS(5),
      CIG_ROWS(5)
    })
  ) {
    for (int i = 0; i < 25; ++i) {
      int row = (i / 5);
      int column = i - (row * 5);
      CIG(_) {
        TEST_ASSERT_EQUAL_RECT(cig_r_make(128*column, 96*row, 128, 96), cig_rect());
      }
    }
  }
}

TEST(core_macros, allocator) {
  int *i = CIG_ALLOCATE(int);
  *i = 5;
  TEST_ASSERT_EQUAL_INT(5, *i);
}

TEST_GROUP_RUNNER(core_macros) {
  RUN_TEST_CASE(core_macros, cig);
  RUN_TEST_CASE(core_macros, vstack);
  RUN_TEST_CASE(core_macros, hstack);
  RUN_TEST_CASE(core_macros, grid);
  RUN_TEST_CASE(core_macros, allocator);
  // TODO: check positional and rect macros
}