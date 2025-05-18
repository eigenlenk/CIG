#include "unity.h"
#include "fixture.h"
#include "ciggfx.h"
#include "cigcorem.h"
#include "asserts.h"
#include "utf8.h"

/* Unit testing image display. Software development just makes sense. */
TEST_GROUP(gfx_image);

static cig_context ctx;
static int test_image = 1;
static cig_r image_rect;

static void draw_image(
  cig_buffer_ref buffer,
  cig_r container,
  cig_r rect,
  cig_image_ref image,
  cig_image_mode mode
) {
  image_rect = rect;
}

static cig_v measure_image(cig_image_ref image) {
  return cig_v_make(80, 60);
}

TEST_SETUP(gfx_image) {
  cig_init_context(&ctx);
  cig_assign_draw_image(&draw_image);
  cig_assign_measure_image(&measure_image);
  cig_begin_layout(&ctx, NULL, cig_r_make(0, 0, 640, 480), 0.1f);
}

TEST_TEAR_DOWN(gfx_image) {}

/*  ┌────────────┐
    │ TEST CASES │
    └────────────┘ */

TEST(gfx_image, aspect_fit) {
  CIG(RECT_SIZED(60, 100)) { /* Portrait container */
    cig_draw_image(&test_image, CIG_IMAGE_MODE_ASPECT_FIT);
    TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 27, 60, 45), image_rect);
  }

  CIG(RECT_SIZED(100, 50)) { /* Landscape container */
    cig_draw_image(&test_image, CIG_IMAGE_MODE_ASPECT_FIT);
    TEST_ASSERT_EQUAL_RECT(cig_r_make(16, 0, 67, 50), image_rect);
  }
}

TEST(gfx_image, aspect_fill) {
  CIG(RECT_SIZED(60, 100)) { /* Portrait container */
    cig_draw_image(&test_image, CIG_IMAGE_MODE_ASPECT_FILL);
    TEST_ASSERT_EQUAL_RECT(cig_r_make(-36, 0, 133, 100), image_rect);
  }

  CIG(RECT_SIZED(100, 50)) { /* Landscape container */
    cig_draw_image(&test_image, CIG_IMAGE_MODE_ASPECT_FILL);
    TEST_ASSERT_EQUAL_RECT(cig_r_make(0, -12, 100, 75), image_rect);
  }
}

TEST(gfx_image, scale_to_fill) {
  cig_draw_image(&test_image, CIG_IMAGE_MODE_SCALE_TO_FILL);
  TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 0, 640, 480), image_rect);
}

TEST(gfx_image, positional_modes) {
  CIG(RECT_SIZED(100, 100)) {
    cig_draw_image(&test_image, cig_image_modeOP_LEFT);
    TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 0, 80, 60), image_rect);

    cig_draw_image(&test_image, cig_image_modeOP);
    TEST_ASSERT_EQUAL_RECT(cig_r_make(10, 0, 80, 60), image_rect);

    cig_draw_image(&test_image, cig_image_modeOP_RIGHT);
    TEST_ASSERT_EQUAL_RECT(cig_r_make(20, 0, 80, 60), image_rect);

    cig_draw_image(&test_image, CIG_IMAGE_MODE_LEFT);
    TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 20, 80, 60), image_rect);

    cig_draw_image(&test_image, CIG_IMAGE_MODE_CENTER);
    TEST_ASSERT_EQUAL_RECT(cig_r_make(10, 20, 80, 60), image_rect);

    cig_draw_image(&test_image, CIG_IMAGE_MODE_RIGHT);
    TEST_ASSERT_EQUAL_RECT(cig_r_make(20, 20, 80, 60), image_rect);
    
    cig_draw_image(&test_image, CIG_IMAGE_MODE_BOTTOM_LEFT);
    TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 40, 80, 60), image_rect);

    cig_draw_image(&test_image, CIG_IMAGE_MODE_BOTTOM);
    TEST_ASSERT_EQUAL_RECT(cig_r_make(10, 40, 80, 60), image_rect);

    cig_draw_image(&test_image, CIG_IMAGE_MODE_BOTTOM_RIGHT);
    TEST_ASSERT_EQUAL_RECT(cig_r_make(20, 40, 80, 60), image_rect);
  }
}

TEST_GROUP_RUNNER(gfx_image) {
  RUN_TEST_CASE(gfx_image, aspect_fit);
  RUN_TEST_CASE(gfx_image, aspect_fill);
  RUN_TEST_CASE(gfx_image, scale_to_fill);
  RUN_TEST_CASE(gfx_image, positional_modes);
}