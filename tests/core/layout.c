#include "unity.h"
#include "fixture.h"
#include "cigcore.h"
#include "asserts.h"

TEST_GROUP(core_layout);

static cig_context_t ctx = { 0 };
static int main_buffer = 1;

TEST_SETUP(core_layout) {
  /* This only needs to be called once, not on every tick */
  cig_init_context(&ctx);

  /* Begin laying out a screen that's 640 by 480 */
  cig_begin_layout(&ctx, &main_buffer, cig_r_make(0, 0, 640, 480), 0.1f);
}

TEST_TEAR_DOWN(core_layout) {
  cig_end_layout();
}

/*  ┌────────────┐
    │ TEST CASES │
    └────────────┘ */

TEST(core_layout, basic_checks) {
  TEST_ASSERT_NOT_NULL(cig_frame());
  TEST_ASSERT_EQUAL(&main_buffer, cig_buffer());
  TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 0, 640, 480), cig_frame()->rect);
  TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 0, 640, 480), cig_frame()->clipped_rect);
  TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 0, 640, 480), cig_frame()->absolute_rect);
  TEST_ASSERT(cig_frame()->insets.left == 0 && cig_frame()->insets.top == 0 &&
    cig_frame()->insets.right == 0 && cig_frame()->insets.bottom == 0);
}

TEST(core_layout, default_insets) {
  cig_set_default_insets(cig_i_uniform(3));

  cig_push_frame(RECT_AUTO);
  TEST_ASSERT(cig_frame()->insets.left == 3 && cig_frame()->insets.top == 3 &&
    cig_frame()->insets.right == 3 && cig_frame()->insets.bottom == 3);
  cig_pop_frame();
}

TEST(core_layout, push_pop) {
  TEST_ASSERT_EQUAL(cig_depth(), 1); /* Just the root */

  cig_push_frame(RECT_AUTO);
    cig_push_frame(RECT_AUTO);
      cig_push_frame(RECT_AUTO);

        TEST_ASSERT_EQUAL(cig_depth(), 4);

      cig_pop_frame();
    cig_pop_frame();

    TEST_ASSERT_EQUAL(cig_depth(), 2);

    cig_push_frame(RECT_AUTO);

      TEST_ASSERT_EQUAL(cig_depth(), 3);

    cig_pop_frame();
  cig_pop_frame();

  TEST_ASSERT_EQUAL(cig_depth(), 1); /* Just the root again */
}

static struct {
  int n;
  cig_id_t recorded[2048];
} ids = { 0 };

static bool assert_unique(const cig_id_t id, int line) {
  register int i;
  for (i = 0; i < ids.n; ++i) {
    if (ids.recorded[i] == id) {
      char failure_message[128];
      sprintf(failure_message, "Line %d: ID %lu is not unique! [%d] = %lu", line, id, i, ids.recorded[i]);
      TEST_FAIL_MESSAGE(failure_message);
      break;
    }
  }
  return true;
}

TEST(core_layout, identifiers) {
  register int a, b, c, d;

  TEST_ASSERT_EQUAL_UINT32(2090695081l, cig_frame()->id); // cig_hash("root")

  const int n0 = 4;

  for (a = 0; a < n0; ++a) {
    if (cig_push_frame(RECT_AUTO)) {
      assert_unique(cig_frame()->id, __LINE__);
      ids.recorded[ids.n++] = cig_frame()->id;
      const int n1 = 2;
      for (b = 0; b < n1; ++b) {
        if (cig_push_frame(RECT_AUTO)) {
          assert_unique(cig_frame()->id, __LINE__);
          ids.recorded[ids.n++] = cig_frame()->id;
          const int n2 = 2;
          for (c = 0; c < n2; ++c) {
            if (cig_push_frame(RECT_AUTO)) {
              assert_unique(cig_frame()->id, __LINE__);
              ids.recorded[ids.n++] = cig_frame()->id;
              const int n3 = 2;
              for (d = 0; d < n3; ++d) {
                if (cig_push_frame(RECT_AUTO)) {
                  assert_unique(cig_frame()->id, __LINE__);
                  ids.recorded[ids.n++] = cig_frame()->id;
                  cig_pop_frame();
                }
              }
              cig_pop_frame();
            }
          }
          cig_pop_frame();
        }
      }
      cig_pop_frame();
    }
  }

  /*  In special cases you can specify the next ID to be used */
  cig_set_next_id(333l);
  cig_push_frame(RECT_AUTO);

  TEST_ASSERT_EQUAL_UINT32(333l, cig_frame()->id);
}

TEST(core_layout, limits) {
  /*  We can insert a total of 2 elements into this one.
      Further push_frame calls will return FALSE */
  cig_push_frame_insets_params(RECT_AUTO, cig_i_zero(), (cig_layout_params_t) {
    .limit.total = 2
  });

  if (cig_push_frame(RECT_AUTO)) { cig_pop_frame(); }
  if (cig_push_frame(RECT_AUTO)) { cig_pop_frame(); }

  if (cig_push_frame(RECT_AUTO)) { TEST_FAIL_MESSAGE("Limit exceeded"); }

  cig_pop_frame();
}

TEST(core_layout, min_max_size) {
  /* Any child of this frame cannot exceed these bounds */
  cig_push_frame_insets_params(RECT_AUTO, cig_i_zero(), (cig_layout_params_t) {
    .size_max.width = 200,
    .size_max.height = 150,
    .size_min.width = 50,
    .size_min.height = 30,
  });

  cig_push_frame(cig_r_make(0, 0, 220, 180));
  TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 0, 200, 150), cig_frame()->rect);
  cig_pop_frame();

  cig_push_frame(cig_r_make(0, 0, 40, 20));
  TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 0, 50, 30), cig_frame()->rect);
  cig_pop_frame();

  cig_push_frame(cig_r_make(0, 0, 100, 100));
  TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 0, 100, 100), cig_frame()->rect);
  cig_pop_frame();

  cig_pop_frame();
}

TEST(core_layout, insets) {
  /* We are changing root frame insets directly. Insets only apply to the children */
  cig_frame()->insets = cig_i_uniform(10);
  
  /* The next frame should be smaller by 10 units on each side as set by its parent.
  `RECT_AUTO` instructs the frame to calculate its size automatically based on where it's being inserted */
  cig_push_frame_insets(RECT_AUTO, cig_i_make(50, 0, 0, 0));
  
  /* Top left origin (X, Y) remains at zero because padding doesn't change
  the coordinates directly - it's applied when calculating the absolute (on-screen)
  frame when rendering. Width and height, however, already take padding(s) into account */
  TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 0, 620, 460), cig_frame()->rect);
  TEST_ASSERT_EQUAL_RECT(cig_r_make(10, 10, 620, 460), cig_absolute_rect());
  
  /* Push another frame. This time there's padding only on the left as set by the previously pushed frame */
  cig_push_frame(RECT_AUTO);
  
  TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 0, 570, 460), cig_frame()->rect);
  TEST_ASSERT_EQUAL_RECT(cig_r_make(60, 10, 570, 460), cig_absolute_rect());
  
  /* Another relative frame, this time off-set from the origin */
  cig_push_frame(cig_r_make(30, 40, 100, 100));
  
  TEST_ASSERT_EQUAL_RECT(cig_r_make(30, 40, 100, 100), cig_frame()->rect);
  TEST_ASSERT_EQUAL_RECT(cig_r_make(90, 50, 100, 100), cig_absolute_rect());
  
  /* Here's what our layout looks like:

  ┌0x0─(Root)──────────────────────640w┐   
  │                 10                 │   
  │  ┌0x0─(First)───────────────620w┐  │
  │  │                  0           │  │ 
  │  │        ┌0x0─(Second)───570w┐ │  │   
  │  │        │                   │ │  │   
  │  │        │   ┌30x40──┐       │ │  │   
  │  │        │   │       │       │0│  │   
  │10│ <-50-> │   │       │       │ │10│   
  │  │        │   │       │       4 │  │   
  │  │        │   └───────┘       6 │  │   
  │  │        │                   0 4  │   
  │  │        └───────────────────┘ 6  │
  │  │                  0           0  4
  │  └──────────────────────────────┘  8   
  │                 10                 0   
  └────────────────────────────────────┘ */
}

TEST(core_layout, overlay) {
  /* Frames don't have to be nested to be overlap or appear to be contained */
  
  /* In this case relative and absolute frames are the same because they're
  directly in root's coordinate system */
  cig_push_frame(cig_r_make(50, 50, 540, 380));
  
  TEST_ASSERT_EQUAL_RECT(cig_r_make(50, 50, 540, 380), cig_frame()->rect);
  TEST_ASSERT_EQUAL_RECT(cig_r_make(50, 50, 540, 380), cig_absolute_rect());
  
  cig_pop_frame();
  
  cig_push_frame(cig_r_make(100, 100, 440, 280));
  
  TEST_ASSERT_EQUAL_RECT(cig_r_make(100, 100, 440, 280), cig_frame()->rect);
  TEST_ASSERT_EQUAL_RECT(cig_r_make(100, 100, 440, 280), cig_absolute_rect());
  
  cig_pop_frame();
  
  /* On screen these 2 frames would appear nested but they're not. They could be
  modal windows for example:

  ╔═════════════════════════════════╗  
  ║                                 ║  
  ║  ┌[Window]────────────────[x]┐  ║  
  ║  │                           │  ║  
  ║  │    ┌[Window]──────[x]┐    │  ║  
  ║  │    │                 │    │  ║  
  ║  │    │                 │    │  ║  
  ║  │    │                 │    │  ║  
  ║  │    │                 │    │  ║  
  ║  │    │                 │    │  ║  
  ║  │    └─────────────────┘    │  ║  
  ║  │                           │  ║  
  ║  └───────────────────────────┘  ║  
  ║                                 ║  
  ╚═════════════════════════════════╝ */
}

TEST(core_layout, culling) {
  /*
  Frames that are wholly outside of visible area are not added.
  `cig_push_frame_*` functions return a BOOL for that. If the frame
  is added successfully you are expected to pop it at some point.
  */
  
  /* This one is wholly outside the parent's top edge */
  if (cig_push_frame(cig_r_make(0, -50, 100, 50))) {
    TEST_FAIL_MESSAGE("Frame should have been culled");
  }
  
  /* This one is wholly outside the parent's right edge */
  if (cig_push_frame(cig_r_make(640, 0, 100, 50))) {
    TEST_FAIL_MESSAGE("Frame should have been culled");
  }
  
  /* This one partially intersects the parent */
  if (cig_push_frame(cig_r_make(0, -25, 100, 50))) {
    cig_pop_frame();
  } else {
    TEST_FAIL_MESSAGE("Frame should NOT have been culled");
  }
  
  /* Culling is enabled by default. We can disable it (for current element) */
  cig_disable_culling();
  
  /* Even though it lays outside the parent, it's still added */
  if (cig_push_frame(cig_r_make(0, -50, 100, 25))) {
    cig_pop_frame();
  } else {
    TEST_FAIL_MESSAGE("Frame should NOT have been culled");
  }
}

TEST(core_layout, content_bounds) {
  TEST_ASSERT_EQUAL_RECT(cig_r_zero(), cig_content_rect());

  if (cig_push_frame(cig_r_make(-35, -25, 100, 50))) { cig_pop_frame(); }
  TEST_ASSERT_EQUAL_RECT(cig_r_make(-35, -25, 100, 50), cig_content_rect());

  if (cig_push_frame(cig_r_make(600, 400, 100, 100))) { cig_pop_frame(); }
  TEST_ASSERT_EQUAL_RECT(cig_r_make(-35, -25, 735, 525), cig_content_rect());
}

TEST(core_layout, vstack_layout) {
  /* Pushing a stack that lays out frames vertically with a 10pt spacing */
  if (!cig_push_layout_function(&cig_default_layout_builder, RECT_AUTO, cig_i_zero(), (cig_layout_params_t) {
    .axis = CIG_LAYOUT_AXIS_VERTICAL,
    .spacing = 10,
    .limit.vertical = 2
  })) {
    TEST_FAIL_MESSAGE("Unable to add layout builder frame");
  }
  
  /* Width is calculated, but height is fixed at 50pt */
  cig_push_frame(RECT_AUTO_H(50));
  TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 0, 640, 50), cig_frame()->rect);
  cig_pop_frame();
  
  cig_push_frame(RECT_AUTO_H(100));
  TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 50+10, 640, 100), cig_frame()->rect);
  cig_pop_frame();
  
  TEST_ASSERT_EQUAL_INT(170, cig_frame()->_layout_params._v_pos);
  
  if (cig_push_frame(RECT_AUTO)) { TEST_FAIL_MESSAGE("Vertical limit exceeded"); }
  
  cig_pop_frame(); /* Not really necessary in testing, but.. */
}

TEST(core_layout, hstack_layout) {
  /* Pushing a stack that lays out frames horizontally with no spacing, but everything is inset by 10pt */
  if (!cig_push_layout_function(&cig_default_layout_builder, RECT_AUTO, cig_i_uniform(10), (cig_layout_params_t) {
    .axis = CIG_LAYOUT_AXIS_HORIZONTAL,
    .spacing = 0
  })) {
    TEST_FAIL_MESSAGE("Unable to add layout builder frame");
  }
  
  /* Width is calculated, but height is fixed at 50pt */
  cig_push_frame(RECT_AUTO_W(200));
  TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 0, 200, 480-2*10), cig_frame()->rect);
  cig_pop_frame();
  
  cig_push_frame(RECT_AUTO_W(100));
  TEST_ASSERT_EQUAL_RECT(cig_r_make(200, 0, 100, 480-2*10), cig_frame()->rect);
  cig_pop_frame();
  
  TEST_ASSERT_EQUAL_INT(300, cig_frame()->_layout_params._h_pos);
  
  cig_pop_frame(); /* Not really necessary in testing, but.. */
}

TEST(core_layout, hstack_mix) {
  if (!cig_push_layout_function(&cig_default_layout_builder, RECT_AUTO, cig_i_zero(), (cig_layout_params_t) {
    .axis = CIG_LAYOUT_AXIS_HORIZONTAL,
    .columns = 4
  })) {
    TEST_FAIL_MESSAGE("Unable to add layout builder frame");
  }

  if (cig_push_frame(RECT_AUTO)) {
    TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 0, 160, 480), cig_frame()->rect);
    cig_pop_frame();
  }

  if (cig_push_frame(RECT_AUTO_W(100))) {
    TEST_ASSERT_EQUAL_RECT(cig_r_make(160, 0, 100, 480), cig_frame()->rect);
    cig_pop_frame();
  }

  if (cig_push_frame(RECT_AUTO)) {
    TEST_ASSERT_EQUAL_RECT(cig_r_make(260, 0, 160, 480), cig_frame()->rect);
    cig_pop_frame();
  }

  if (cig_push_frame(RECT_AUTO)) {
    TEST_ASSERT_EQUAL_RECT(cig_r_make(420, 0, 160, 480), cig_frame()->rect);
    cig_pop_frame();
  }

  cig_pop_frame();
}

TEST(core_layout, vstack_align_bottom) {
  if (!cig_push_layout_function(&cig_default_layout_builder, RECT_AUTO, cig_i_zero(), (cig_layout_params_t) {
    .axis = CIG_LAYOUT_AXIS_VERTICAL,
    .alignment.vertical = CIG_LAYOUT_ALIGNS_BOTTOM,
    .height = 50,
    .spacing = 0
  })) {
    TEST_FAIL_MESSAGE("Unable to add layout builder frame");
  }

  for (int i = 0; i < 2; ++i) {
    if (cig_push_frame(RECT_AUTO)) {
      TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 480-50-(i*50), 640, 50), cig_frame()->rect);
      cig_pop_frame();
    }
  }

  cig_frame()->_layout_params.height = 0;

  cig_push_frame(RECT_AUTO);

  TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 0, 640, 380), cig_frame()->rect);
}

TEST(core_layout, hstack_align_right) {
  if (!cig_push_layout_function(&cig_default_layout_builder, RECT_AUTO, cig_i_zero(), (cig_layout_params_t) {
    .axis = CIG_LAYOUT_AXIS_HORIZONTAL,
    .alignment.horizontal = CIG_LAYOUT_ALIGNS_RIGHT,
    .width = 50,
    .spacing = 0
  })) {
    TEST_FAIL_MESSAGE("Unable to add layout builder frame");
  }

  for (int i = 0; i < 8; ++i) {
    if (cig_push_frame(RECT_AUTO)) {
      TEST_ASSERT_EQUAL_RECT(cig_r_make(640-50-(i*50), 0, 50, 480), cig_frame()->rect);
      cig_pop_frame();
    }
  }
}

TEST(core_layout, standard_frame_alignment) {
  /*  Alignment options can be used for standard frames as well */
  if (!cig_push_layout_function(&cig_default_layout_builder, RECT_AUTO, cig_i_zero(), (cig_layout_params_t) {
    .alignment.horizontal = CIG_LAYOUT_ALIGNS_RIGHT,
    .alignment.vertical = CIG_LAYOUT_ALIGNS_BOTTOM
  })) {
    TEST_FAIL_MESSAGE("Unable to add layout builder frame");
  }

  /*  Bottom-right alignment means (0,0) is at the bottom-right corner of the parent rectangle */
  cig_push_frame(cig_r_make(0, 0, 200, 100));
  TEST_ASSERT_EQUAL_RECT(cig_r_make(440, 380, 200, 100), cig_frame()->rect);
}

/* - Grid is the same stack layout builder but with both axis enabled.
   - Grid will start adding frames horizontally/vertically, until the position exceeds
     the width/height or when the next proposed frame can't fit.
     Then the position moves to the next row/column */

TEST(core_layout, grid_with_fixed_rows_and_columns) {
  /* We are specifying a number of rows and columns - this will tell
     how large each child needs to be by default (we *can* override).
     Here it's a 5x5 grid, meaning on our 640x480 screen,
     each cell would be 128x96 */
  if (!cig_push_layout_function(&cig_default_layout_builder, RECT_AUTO, cig_i_zero(), (cig_layout_params_t) {
    .axis = CIG_LAYOUT_AXIS_HORIZONTAL | CIG_LAYOUT_AXIS_VERTICAL,
    .spacing = 0,
    .columns = 5,
    .rows = 5
  })) {
    TEST_FAIL_MESSAGE("Unable to add layout builder frame");
  }
  
  int i;
  for (i = 0; i < 25; ++i) {
    int row = (i / 5);
    int column = i - (row * 5);
    cig_push_frame(RECT_AUTO);
    TEST_ASSERT_EQUAL_RECT(cig_r_make(128*column, 96*row, 128, 96), cig_frame()->rect);
    cig_pop_frame();
  }

  TEST_ASSERT_EQUAL_INT(0, cig_frame()->_layout_params._h_pos);
  TEST_ASSERT_EQUAL_INT(480, cig_frame()->_layout_params._v_pos);
}

TEST(core_layout, grid_with_fixed_cell_size) {
  /* Here we are defining a grid where each cell's width and height is fixed.
     If columns and rows are not set, there will be as many cells horizontally
     as could be fitted, in this case 640 / 200 = 3 and the remaining space will
     be unused. Same vertically (480 / 200) = 2, 6 in total, looking something like this:
  
     ┌─────────────────────────────────┐
     │┌────────┐┌────────┐┌────────┐...│
     ││        ││        ││        │...│
     ││        ││        ││        │...│
     ││        ││        ││        │...│
     │└────────┘└────────┘└────────┘...│
     │┌────────┐┌────────┐┌────────┐...│
     ││        ││        ││        │...│
     ││        ││        ││        │...│
     ││        ││        ││        │...│
     │└────────┘└────────┘└────────┘...│
     │.................................│
     └─────────────────────────────────┘ */
  if (!cig_push_layout_function(&cig_default_layout_builder, RECT_AUTO, cig_i_zero(), (cig_layout_params_t) {
    .axis = CIG_LAYOUT_AXIS_HORIZONTAL | CIG_LAYOUT_AXIS_VERTICAL,
    .width = 200,
    .height = 200
  })) {
    TEST_FAIL_MESSAGE("Unable to add layout builder frame");
  }
  
  int i;
  
  /* First row */
  for (i = 0; i < 3; ++i) {
    cig_push_frame(RECT_AUTO);
    cig_pop_frame();
  }
  
  TEST_ASSERT_EQUAL_INT(600, cig_frame()->_layout_params._h_pos);
  
  /* Second row */
  for (i = 0; i < 3; ++i) {
    cig_push_frame(RECT_AUTO);
    TEST_ASSERT_EQUAL_INT(200, cig_frame()->rect.y);
    cig_pop_frame();
  }
  
  TEST_ASSERT_EQUAL_INT(200, cig_frame()->_layout_params._v_pos);
}

TEST(core_layout, grid_with_varying_cell_size) {
  /* Third option is to specify a size for each of the cells at insertion time.
     Then, again depending on the remaining space, cell will be inserted into the
     current row or pushed to the next. In addition, you can still specify the number
     of rows and columns, and these will now be used to limit number of cells on each axis */
  if (!cig_push_layout_function(&cig_default_layout_builder, RECT_AUTO, cig_i_zero(), (cig_layout_params_t) {
    .axis = CIG_LAYOUT_AXIS_HORIZONTAL | CIG_LAYOUT_AXIS_VERTICAL,
    .limit.horizontal = 3
  })) {
    TEST_FAIL_MESSAGE("Unable to add layout builder frame");
  }

  /* Add 3 elements with increasing width. They should fit on the first row */
  cig_push_frame(RECT_SIZED(100, 160)); /* (1) */
  TEST_ASSERT_EQUAL_INT(0, cig_frame()->rect.x);
  cig_pop_frame();
  
  TEST_ASSERT_EQUAL_INT(100, cig_frame()->_layout_params._h_pos);
  
  cig_push_frame(RECT_SIZED(200, 160)); /* (2) */
  TEST_ASSERT_EQUAL_INT(100, cig_frame()->rect.x);
  cig_pop_frame();
  
  TEST_ASSERT_EQUAL_INT(300, cig_frame()->_layout_params._h_pos);
  
  cig_push_frame(RECT_SIZED(300, 160)); /* (3) */
  TEST_ASSERT_EQUAL_INT(300, cig_frame()->rect.x);
  cig_pop_frame();
  
  TEST_ASSERT_EQUAL_INT(600, cig_frame()->_layout_params._h_pos);
  
  /* Let's try to insert another cell that should fit width wise,
  but the grid would exceed the number of horizontal elements,
  so it's pushed onto the next row */
  cig_push_frame(RECT_SIZED(40, 160)); /* (4) */
  TEST_ASSERT_EQUAL_INT(0,    cig_frame()->rect.x);
  TEST_ASSERT_EQUAL_INT(160,  cig_frame()->rect.y);
  cig_pop_frame();
  
  /* This one should be inserted normally onto the second row */
  cig_push_frame(RECT_SIZED(400, 160)); /* (5) */
  TEST_ASSERT_EQUAL_INT(40,   cig_frame()->rect.x);
  TEST_ASSERT_EQUAL_INT(160,  cig_frame()->rect.y);
  cig_pop_frame();
  
  /* Spacer fills the remaining space on the second row. Internally this is just a frame push + pop */
  cig_spacer(CIG__AUTO_BIT);
  
  /* Insert an element to fill all the space on the third row */
  cig_push_frame(RECT_AUTO); /* (6) */
  TEST_ASSERT_EQUAL_INT(640,  cig_frame()->rect.w);
  TEST_ASSERT_EQUAL_INT(160,  cig_frame()->rect.h);
  cig_pop_frame();
  
  /* For visualisation:
    ┌──────────────────────┐
    │┌──┐┌─────┐┌───────┐..│
    ││1 ││  2  ││   3   │..│ <- (4) could go here but we've
    │└──┘└─────┘└───────┘..│    set a limit on 3 columns, or
    │┌─┐┌────────────┐┌   ┐│    3 elements per row more precisely
    ││4││    5       │     │
    │└─┘└────────────┘└   ┘│
    │┌────────────────────┐│
    ││         6          ││
    │└────────────────────┘│
    └──────────────────────┘ */
}

TEST(core_layout, grid_with_down_direction) {
  /*  Grids support horizontal (default) and vertical layout direction. In vertical mode,
      instead of filling and adding rows, columns are filled and added instead. Otherwise
      they behave the same */
  if (!cig_push_layout_function(&cig_default_layout_builder, RECT_AUTO, cig_i_zero(), (cig_layout_params_t) {
    .axis = CIG_LAYOUT_AXIS_HORIZONTAL | CIG_LAYOUT_AXIS_VERTICAL,
    .direction = CIG_LAYOUT_DIRECTION_VERTICAL
  })) {
    TEST_FAIL_MESSAGE("Unable to add layout builder frame");
  }
  
  /* Add 3 elements with increasing height and width. They should fit in the first column */
  cig_push_frame(RECT_SIZED(100, 120)); /* (1) */
  TEST_ASSERT_EQUAL_INT(0,    cig_frame()->rect.y);
  cig_pop_frame();
  
  TEST_ASSERT_EQUAL_INT(120,  cig_frame()->_layout_params._v_pos);
  
  cig_push_frame(RECT_SIZED(150, 160)); /* (2) */
  TEST_ASSERT_EQUAL_INT(120,  cig_frame()->rect.y);
  cig_pop_frame();
  
  TEST_ASSERT_EQUAL_INT(280,  cig_frame()->_layout_params._v_pos);
  
  cig_push_frame(RECT_SIZED(200, 200)); /* (3) */
  TEST_ASSERT_EQUAL_INT(280,  cig_frame()->rect.y);
  cig_pop_frame();
  
  /* No remaining space vertically - position moves back to the top and to the next column */
  TEST_ASSERT_EQUAL_INT(200,  cig_frame()->_layout_params._h_pos);
  TEST_ASSERT_EQUAL_INT(0,    cig_frame()->_layout_params._v_pos);
  
  /* Without anything else configured on the grid, the next element will fill
     the remaining space on the right */
  cig_push_frame(RECT_AUTO); /* (4) */
  TEST_ASSERT_EQUAL_RECT(cig_r_make(200, 0, 440, 480), cig_frame()->rect);
  cig_pop_frame();
  
  TEST_ASSERT_EQUAL_INT(640,  cig_frame()->_layout_params._h_pos);
  
  /*  For visualisation:
    ┌────────────────────────┐
    │┌───┐....┌─────────────┐│
    ││ 1 │....│             ││
    │└───┘....│             ││
    │┌─────┐..│             ││
    ││  2  │..│             ││
    │└─────┘..│      4      ││
    │┌───────┐│             ││
    ││       ││             ││
    ││   3   ││             ││
    ││       ││             ││
    │└───────┘└─────────────┘│
    └────────────────────────┘ */
}

TEST(core_layout, grid_with_flipped_alignment_and_direction) {
  /*  Grid that aligns to bottom-right corner starts adding elmenents into columns */
  if (!cig_push_layout_function(&cig_default_layout_builder, RECT_AUTO, cig_i_zero(), (cig_layout_params_t) {
    .axis = CIG_LAYOUT_AXIS_HORIZONTAL | CIG_LAYOUT_AXIS_VERTICAL,
    .direction = CIG_LAYOUT_DIRECTION_VERTICAL,
    .alignment.horizontal = CIG_LAYOUT_ALIGNS_RIGHT,
    .alignment.vertical = CIG_LAYOUT_ALIGNS_BOTTOM,
    .width = 200,
    .height = 200
  })) {
    TEST_FAIL_MESSAGE("Unable to add layout builder frame");
  }

  /*  First column starts growing upwards */
  if (cig_push_frame(RECT_AUTO)) { /* 1 */
    TEST_ASSERT_EQUAL_RECT(cig_r_make(640-200*1, 480-200*1, 200, 200), cig_frame()->rect);
    cig_pop_frame(); 
  }

  if (cig_push_frame(RECT_AUTO)) { /* 2 */
    TEST_ASSERT_EQUAL_RECT(cig_r_make(640-200*1, 480-200*2, 200, 200), cig_frame()->rect);
    cig_pop_frame(); 
  }

  /*  First (rightmost) column can't fit another 200H element,
      so it moves onto the next column to the left */
  if (cig_push_frame(RECT_AUTO)) { /* 3 */
    TEST_ASSERT_EQUAL_RECT(cig_r_make(640-200*2, 480-200*1, 200, 200), cig_frame()->rect);
    cig_pop_frame(); 
  }

  /*  For visualisation:
      ┌────────────────────────────┐
      │            ................│
      │            ┌      ┐┌──────┐│
      │             (4)    │ 2    ││
      │                    │      ││
      │                    │      ││
      │            └      ┘└──────┘│
      │            ┌──────┐┌──────┐│
      │            │ 3    ││ 1    ││
      │            │      ││      ││
      │            │      ││      ││
      │            └──────┘└──────┘│
      └────────────────────────────┘ */
}

TEST(core_layout, vstack_scroll) {
  /* Any element can be made scrollable, but it makes most sense for stacks/grids */
  if (!cig_push_layout_function(&cig_default_layout_builder, RECT_AUTO, cig_i_zero(), (cig_layout_params_t) {
    .axis = CIG_LAYOUT_AXIS_VERTICAL,
    .height = 100
  })) {
    TEST_FAIL_MESSAGE("Unable to add layout builder frame");
  }

  /*  Scrolling is not enabled by default */
  TEST_ASSERT_NULL(cig_frame()->_scroll_state);
  TEST_ASSERT_FALSE(cig_frame()->_flags & CLIPPED);

  cig_enable_scroll(NULL);

  /* Scrolling also enables clipping */
  TEST_ASSERT_TRUE(cig_frame()->_flags & CLIPPED);

  cig_scroll_state_t *scroll = cig_scroll_state();

  TEST_ASSERT_EQUAL_VEC2(cig_v_zero(), scroll->offset);
  TEST_ASSERT_EQUAL_RECT(cig_r_zero(), cig_content_rect());

  scroll->offset.y = 220;

  /*  Let's add some content to the stack */
  for (int i = 0; i < 10; ++i) {
    if (cig_push_frame(RECT_AUTO)) {
      /* Elements should be offset by scroll amount on the Y axis */
      TEST_ASSERT_EQUAL_INT(i*100-220, cig_frame()->rect.y);
      cig_pop_frame();
    }
  }

  TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 0, 640, 100*10), cig_content_rect());
}

TEST(core_layout, clipping) {
  /*  Clipping is partially a graphical feature implemented in the backend,
      but the layout elements also calculate a relative frame that's been clipped.
     
      Clipping is OFF by default, but every layout element is eventually clipped
      against the root element (the screen itself) because things are clipped
      automatically by most renderers when drawing out of bounds */ 
     
  /*  An element that's partially outside the root bounds gets clipped */
  cig_push_frame(cig_r_make(100, -100, 440, 200));
  TEST_ASSERT_EQUAL_RECT(cig_r_make(100, 0, 440, 100), cig_frame()->clipped_rect);
  cig_pop_frame();
  
  /*  Some element filling the whole root */
  cig_push_frame(RECT_AUTO);

  /*  Clipping is now turned ON for the current layout element */
  cig_enable_clipping();

  cig_push_frame(cig_r_make(600, 400, 100, 100));

  TEST_ASSERT_EQUAL_RECT(cig_r_make(600, 400, 40, 80), cig_frame()->clipped_rect);
    
    cig_push_frame(cig_r_make(30, 70, 20, 20));
    TEST_ASSERT_EQUAL_RECT(cig_r_make(30, 70, 10, 10), cig_frame()->clipped_rect);
    cig_pop_frame();
    
  cig_pop_frame();

  cig_push_frame(cig_r_make(-75, -50, 200, 200));
  
  TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 0, 125, 150), cig_frame()->clipped_rect);
    
    cig_push_frame(cig_r_make(0, 0, 100, 100));
    TEST_ASSERT_EQUAL_RECT(cig_r_make(75, 50, 25, 50), cig_frame()->clipped_rect);
    cig_pop_frame();
    
    cig_push_frame(cig_r_make(-25, -50, 100, 100));
    TEST_ASSERT_EQUAL_RECT(cig_r_make(75, 50, 0, 0), cig_frame()->clipped_rect);
    cig_pop_frame();

  cig_pop_frame();

  cig_push_frame(cig_r_make(100, 100, 440, 280));
  cig_enable_clipping();

    cig_push_frame(cig_r_make(-10, -50, 460, 100));
    TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 0, 440, 50), cig_frame()->clipped_rect);
    cig_pop_frame();

  cig_pop_frame();

  cig_pop_frame();
}

TEST(core_layout, additional_buffers) {
  int secondary_buffer = 2;

  cig_push_frame(cig_r_make(100, 100, 440, 280));
  cig_push_buffer(&secondary_buffer);

  TEST_ASSERT_EQUAL(&secondary_buffer, cig_buffer());
  TEST_ASSERT_EQUAL_RECT(cig_r_make(100, 100, 440, 280), cig_frame()->clipped_rect);
  TEST_ASSERT_EQUAL_RECT(cig_r_make(100, 100, 440, 280), cig_frame()->absolute_rect);

  /*  Another child within that new buffer */
  cig_push_frame(cig_r_make(100, 100, 240, 80));

  TEST_ASSERT_EQUAL_RECT(cig_r_make(100, 100, 240, 80), cig_frame()->rect);

  /*  Absolute frame is now in the coordinate space of the new buffer, rather than the main buffer */
  TEST_ASSERT_EQUAL_RECT(cig_r_make(100, 100, 240, 80), cig_frame()->absolute_rect);

  cig_pop_frame();

  cig_pop_buffer();
  cig_pop_frame();
}

TEST(core_layout, main_screen_subregion) {
  /*  Let's end the original layout added by the test harness .. */
  cig_end_layout();

  /*  .. and start a new UI. Lets imagine we have a larger game screen but only want to render
      the UI in the bottom half of it. So on our 640x480 screen the Y and H would both be 240 */
  cig_begin_layout(&ctx, &main_buffer, cig_r_make(0, 240, 640, 240), 0.1f);

  /*  The UI should be clipped within that larger screen */
  TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 240, 640, 240), cig_frame()->clipped_rect);

  cig_push_frame(RECT_AUTO);

  /*  The absolute frame should be offset as well, while the relative frames stay the same */
  TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 0, 640, 240), cig_frame()->rect);
  TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 0, 640, 240), cig_frame()->clipped_rect);
  TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 240, 640, 240), cig_frame()->absolute_rect);

  cig_pop_frame();
}

TEST(core_layout, relative_values) {
  /*  CIG_REL allows us to create frames with relative sizes */
  if (cig_push_frame(RECT(0, 0, CIG_REL(0.5), CIG_REL(0.75)))) {
    TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 0, 320, 360), cig_frame()->rect);
    cig_pop_frame();
  }

  /*  The same works for X and Y */
  if (cig_push_frame(RECT(-CIG_REL(0.5), CIG_REL(0.5), 100, 100))) {
    TEST_ASSERT_EQUAL_RECT(cig_r_make(-320, 240, 100, 100), cig_frame()->rect);
    cig_pop_frame();
  }

  /*  CIG_REL and CIG_AUTO can also work together. In standard frames
      CIG_AUTO(CIG_REL(0.5)) is equivalent to CIG_REL(0.5),
      but in stacks for example it can yield different values */
  if (cig_push_vstack(RECT_AUTO, cig_i_zero(), (cig_layout_params_t) {
    .height = 70
  })) {
    /*  This stack has 70px rows by default but we can specify we want DOUBLE that.
        This isn't very intuitive though? */
    if (cig_push_frame(RECT(0, 0, CIG_AUTO(), CIG_AUTO(CIG_REL(2.0))))) {
      TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 0, 640, 140), cig_frame()->rect);
      cig_pop_frame();
    }
  }
}

static cig_r clip_rect;
static void set_clip_rect(cig_buffer_ref buffer, cig_r rect, bool is_root) {
  clip_rect = rect;
}

TEST(core_layout, jump) {
  cig_set_clip_rect_callback(&set_clip_rect);

  /*  Jumping is a mechanism to go back to a previously closed element. You can
      continue adding new elements into it. This can be useful in cases where the
      order of laying things out and order in which something is computed differs.

      For example in our Windows 95 demo, the directory explorer window shows a status
      text at the very bottom which comes from calling the content functor. The space
      for that text is allocated before the main content, but the actual text in there
      is added after the fact by jumping back to it. */

  cig_push_frame(cig_r_make(20, 20, 200, 200));
  cig_enable_clipping();
  
  TEST_ASSERT_EQUAL_RECT(cig_r_make(20, 20, 200, 200), clip_rect);
  
  cig_frame_t *my_frame = cig_push_frame(RECT_AUTO);
  /* ... */
  cig_pop_frame();
  
  cig_pop_frame(); /* This pops the clip rect */

  /* We should be back to clipping the whole screen */
  TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 0, 640, 480), clip_rect);

  /*  Now let's jump back to 'my_frame' and add something in there. Jumping
      also restores whatever clip rect was active at that time. */
  cig_jump(my_frame);
  TEST_ASSERT_EQUAL_RECT(cig_r_make(20, 20, 200, 200), clip_rect);
  
  cig_push_frame(RECT_AUTO);
  TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 0, 200, 200), cig_frame()->rect);
  cig_pop_frame();
  
  cig_pop_frame(); /* We pop this like usual */

  /* Again, we should be back to clipping the whole screen */
  TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 0, 640, 480), clip_rect);
}

TEST_GROUP_RUNNER(core_layout) {
  RUN_TEST_CASE(core_layout, basic_checks);
  RUN_TEST_CASE(core_layout, default_insets);
  RUN_TEST_CASE(core_layout, push_pop);
  RUN_TEST_CASE(core_layout, identifiers);
  RUN_TEST_CASE(core_layout, limits);
  RUN_TEST_CASE(core_layout, min_max_size);
  RUN_TEST_CASE(core_layout, insets);
  RUN_TEST_CASE(core_layout, overlay);
  RUN_TEST_CASE(core_layout, culling);
  RUN_TEST_CASE(core_layout, content_bounds);
  RUN_TEST_CASE(core_layout, vstack_layout);
  RUN_TEST_CASE(core_layout, hstack_layout);
  RUN_TEST_CASE(core_layout, hstack_mix);
  RUN_TEST_CASE(core_layout, hstack_align_right);
  RUN_TEST_CASE(core_layout, vstack_align_bottom);
  RUN_TEST_CASE(core_layout, standard_frame_alignment);
  RUN_TEST_CASE(core_layout, grid_with_fixed_rows_and_columns);
  RUN_TEST_CASE(core_layout, grid_with_fixed_cell_size);
  RUN_TEST_CASE(core_layout, grid_with_varying_cell_size);
  RUN_TEST_CASE(core_layout, grid_with_down_direction);
  RUN_TEST_CASE(core_layout, grid_with_flipped_alignment_and_direction);
  RUN_TEST_CASE(core_layout, vstack_scroll);
  RUN_TEST_CASE(core_layout, clipping);
  RUN_TEST_CASE(core_layout, additional_buffers);
  RUN_TEST_CASE(core_layout, main_screen_subregion);
  RUN_TEST_CASE(core_layout, relative_values);
  RUN_TEST_CASE(core_layout, jump)
}
