#include "unity.h"
#include "fixture.h"
#include "cigcore.h"
#include "asserts.h"

TEST_GROUP(core_macros);

static cig_context_t ctx = { 0 };

TEST_SETUP(core_macros) {
  cig_init_context(&ctx);
	cig_begin_layout(&ctx, NULL, cig_rect_make(0, 0, 640, 480));
}

TEST_TEAR_DOWN(core_macros) {
	cig_end_layout();
}

TEST(core_macros, arrange) {
  /* This creates a 500x400 frame with 4 unit inset and centers it in the root frame */
  CIG_ARRANGE_WITH(
    CIG_CENTERED(500, 400),
    cig_insets_uniform(4),
    CIG_PARAMS(),
    CIG_BODY(
      TEST_ASSERT_EQUAL_RECT(cig_rect_make(70, 40, 500, 400), cig_rect());
      
      /* Regular control flow works in here */
      for (int i = 0; i < 2; ++i) { }
     
      CIG_ARRANGE(CIG_FILL, CIG_BODY(
        TEST_ASSERT_EQUAL_RECT(cig_rect_make(0, 0, 492, 392), cig_rect());
      ))
      
      /* No need to use CIG_BODY macro. The additional parenthesis just help with indentation.
         CIG_FILLED(BODY...) === CIG_ARRANGE(CIG_FILL, BODY...) */
      CIG_FILLED(
        TEST_ASSERT_EQUAL_RECT(cig_rect_make(0, 0, 492, 392), cig_rect());
        for (int i = 0; i < 2; ++i) { }
      )
    ))
}

TEST(core_macros, vstack) {
  CIG_VSTACK(
    CIG_FILL,
    CIG_BODY(
      for (int i = 0; i < 2; ++i) {
        CIG_ARRANGE(CIG_FILL, CIG_BODY(
          TEST_ASSERT_EQUAL_RECT(cig_rect_make(0, i*(200+10), 640, 200), cig_rect());
        ))
      }
    ),
    CIG_HEIGHT(200),
    CIG_SPACING(10),
    CIG_LIMIT_VERTICAL(2)
  )
}

TEST(core_macros, hstack) {
  CIG_HSTACK(
    CIG_FILL,
    CIG_BODY(
      for (int i = 0; i < 2; ++i) {
        CIG_ARRANGE(CIG_FILL, CIG_BODY(
          TEST_ASSERT_EQUAL_RECT(cig_rect_make(i*(200+10), 0, 200, 480), cig_rect());
        ))
      }
    ),
    CIG_WIDTH(200),
    CIG_SPACING(10),
    CIG_LIMIT_HORIZONTAL(2)
  )
}

TEST(core_macros, grid) {
  /* 5x5 grid with 0 spacing */
  CIG_GRID(CIG_FILL, 5, 5, 0, CIG_BODY(
    for (int i = 0; i < 25; ++i) {
      int row = (i / 5);
      int column = i - (row * 5);
      CIG_FILLED(
        TEST_ASSERT_EQUAL_RECT(cig_rect_make(128*column, 96*row, 128, 96), cig_rect());
      )
    }
  ))
}

TEST_GROUP_RUNNER(core_macros) {
  RUN_TEST_CASE(core_macros, arrange);
  RUN_TEST_CASE(core_macros, vstack);
  RUN_TEST_CASE(core_macros, hstack);
  RUN_TEST_CASE(core_macros, grid);
}