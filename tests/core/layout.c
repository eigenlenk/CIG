#include "unity.h"
#include "fixture.h"
#include "cigcore.h"
#include "asserts.h"

TEST_GROUP(core_layout);

static cig_context ctx = { 0 };
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
  TEST_ASSERT_NOT_NULL(cig_current());
  TEST_ASSERT_EQUAL(&main_buffer, cig_buffer());
  TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 0, 640, 480), cig_current()->rect);
  TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 0, 640, 480), cig_current()->clipped_rect);
  TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 0, 640, 480), cig_current()->absolute_rect);
  TEST_ASSERT_FALSE(cig_is_vertical_layout());
  TEST_ASSERT(cig_current()->insets.left == 0 && cig_current()->insets.top == 0 &&
    cig_current()->insets.right == 0 && cig_current()->insets.bottom == 0);
}

TEST(core_layout, default_insets) {
  cig_set_default_insets(cig_i_uniform(3));

  cig_push_frame(RECT_AUTO);
  TEST_ASSERT(cig_current()->insets.left == 3 && cig_current()->insets.top == 3 &&
    cig_current()->insets.right == 3 && cig_current()->insets.bottom == 3);
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
  cig_id recorded[2048];
} ids = { 0 };

static bool assert_unique(const cig_id id, int line) {
  register int i;
  for (i = 0; i < ids.n; ++i) {
    if (ids.recorded[i] == id) {
      char failure_message[128];
      sprintf(failure_message, "Line %d: ID %llx is not unique! [%d] = %llx", line, (unsigned long long)id, i, (unsigned long long)ids.recorded[i]);
      TEST_FAIL_MESSAGE(failure_message);
      break;
    }
  }
  return true;
}

TEST(core_layout, identifiers) {
  register int a, b, c, d;

  TEST_ASSERT_EQUAL_UINT32(2090695081l, cig_current()->id); // cig_hash("root")

  const int n0 = 4;

  for (a = 0; a < n0; ++a) {
    if (cig_push_frame(RECT_AUTO)) {
      assert_unique(cig_current()->id, __LINE__);
      ids.recorded[ids.n++] = cig_current()->id;
      const int n1 = 2;
      for (b = 0; b < n1; ++b) {
        if (cig_push_frame(RECT_AUTO)) {
          assert_unique(cig_current()->id, __LINE__);
          ids.recorded[ids.n++] = cig_current()->id;
          const int n2 = 2;
          for (c = 0; c < n2; ++c) {
            if (cig_push_frame(RECT_AUTO)) {
              assert_unique(cig_current()->id, __LINE__);
              ids.recorded[ids.n++] = cig_current()->id;
              const int n3 = 2;
              for (d = 0; d < n3; ++d) {
                if (cig_push_frame(RECT_AUTO)) {
                  assert_unique(cig_current()->id, __LINE__);
                  ids.recorded[ids.n++] = cig_current()->id;
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

  TEST_ASSERT_EQUAL_UINT32(333l, cig_current()->id);
}

TEST(core_layout, limits) {
  /*  We can insert a total of 2 elements into this one.
      Further push_frame calls will return FALSE */
  cig_push_frame_insets_params(RECT_AUTO, cig_i_zero(), (cig_params) {
    .limit.total = 2
  });

  if (cig_push_frame(RECT_AUTO)) { cig_pop_frame(); }
  if (cig_push_frame(RECT_AUTO)) { cig_pop_frame(); }

  if (cig_push_frame(RECT_AUTO)) { TEST_FAIL_MESSAGE("Limit exceeded"); }

  cig_pop_frame();
}

TEST(core_layout, min_max_size) {
  /* Any child of this frame cannot exceed these bounds */
  cig_push_frame_insets_params(RECT_AUTO, cig_i_zero(), (cig_params) {
    .size_max.width = 200,
    .size_max.height = 150,
    .size_min.width = 50,
    .size_min.height = 30,
  });

  cig_push_frame(cig_r_make(0, 0, 220, 180));
  TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 0, 200, 150), cig_current()->rect);
  cig_pop_frame();

  cig_push_frame(cig_r_make(0, 0, 40, 20));
  TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 0, 50, 30), cig_current()->rect);
  cig_pop_frame();

  cig_push_frame(cig_r_make(0, 0, 100, 100));
  TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 0, 100, 100), cig_current()->rect);
  cig_pop_frame();

  cig_pop_frame();
}

TEST(core_layout, insets) {
  /* We are changing root frame insets directly. Insets only apply to the children */
  cig_current()->insets = cig_i_uniform(10);
  
  /* The next frame should be smaller by 10 units on each side as set by its parent.
  `RECT_AUTO` instructs the frame to calculate its size automatically based on where it's being inserted */
  cig_push_frame_insets(RECT_AUTO, cig_i_make(50, 0, 0, 0));
  
  /* Top left origin (X, Y) remains at zero because padding doesn't change
  the coordinates directly - it's applied when calculating the absolute (on-screen)
  frame when rendering. Width and height, however, already take padding(s) into account */
  TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 0, 620, 460), cig_current()->rect);
  TEST_ASSERT_EQUAL_RECT(cig_r_make(10, 10, 620, 460), cig_absolute_rect());
  
  /* Push another frame. This time there's padding only on the left as set by the previously pushed frame */
  cig_push_frame(RECT_AUTO);
  
  TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 0, 570, 460), cig_current()->rect);
  TEST_ASSERT_EQUAL_RECT(cig_r_make(60, 10, 570, 460), cig_absolute_rect());
  
  /* Another relative frame, this time off-set from the origin */
  cig_push_frame(cig_r_make(30, 40, 100, 100));
  
  TEST_ASSERT_EQUAL_RECT(cig_r_make(30, 40, 100, 100), cig_current()->rect);
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
  
  TEST_ASSERT_EQUAL_RECT(cig_r_make(50, 50, 540, 380), cig_current()->rect);
  TEST_ASSERT_EQUAL_RECT(cig_r_make(50, 50, 540, 380), cig_absolute_rect());
  
  cig_pop_frame();
  
  cig_push_frame(cig_r_make(100, 100, 440, 280));
  
  TEST_ASSERT_EQUAL_RECT(cig_r_make(100, 100, 440, 280), cig_current()->rect);
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
  Frames that are wholly outside of the parent frame are not added. For that
  `cig_push_frame_*` functions return a `cig_frame` reference or NULL. If the frame
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

/* Each frame tracks a content rect that bounds all direct child elements */
TEST(core_layout, content_bounds) {
  TEST_ASSERT_EQUAL_RECT(cig_r_zero(), cig_content_rect());

  if (cig_push_frame(cig_r_make(-35, -25, 100, 50))) { cig_pop_frame(); }
  TEST_ASSERT_EQUAL_RECT(cig_r_make(-35, -25, 100, 50), cig_content_rect());

  if (cig_push_frame(cig_r_make(600, 400, 100, 100))) { cig_pop_frame(); }
  TEST_ASSERT_EQUAL_RECT(cig_r_make(-35, -25, 735, 525), cig_content_rect());
}

TEST(core_layout, vstack_layout) {
  /* Pushing a stack that lays out frames vertically with a 10pt spacing */
  if (!cig_push_layout_function(&cig_default_layout_builder, RECT_AUTO, cig_i_zero(), (cig_params) {
    .axis = CIG_LAYOUT_AXIS_VERTICAL,
    .spacing = 10,
    .limit.vertical = 2
  })) {
    TEST_FAIL_MESSAGE("Unable to add layout builder frame");
  }

  TEST_ASSERT_TRUE(cig_is_vertical_layout());
  
  cig_push_frame(RECT_AUTO_H(50)); /* RECT_AUTO_H: Width is calculated, but height is fixed at 50pt */
  TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 0, 640, 50), cig_current()->rect);
  cig_pop_frame();
  
  cig_push_frame(RECT_AUTO_H(100));
  TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 50+10, 640, 100), cig_current()->rect);
  cig_pop_frame();
  
  TEST_ASSERT_EQUAL_INT(170, cig_current()->_layout_params._v_pos);
  
  if (cig_push_frame(RECT_AUTO)) { TEST_FAIL_MESSAGE("Vertical limit exceeded"); }
  
  cig_pop_frame(); /* Not really necessary in testing, but.. */
}

TEST(core_layout, hstack_layout) {
  /* Pushing a stack that lays out frames horizontally with no spacing, but everything is inset by 10pt */
  if (!cig_push_layout_function(&cig_default_layout_builder, RECT_AUTO, cig_i_uniform(10), (cig_params) {
    .axis = CIG_LAYOUT_AXIS_HORIZONTAL,
    .spacing = 0
  })) {
    TEST_FAIL_MESSAGE("Unable to add layout builder frame");
  }
  
  TEST_ASSERT_FALSE(cig_is_vertical_layout());

  cig_push_frame(RECT_AUTO_W(200)); /* RECT_AUTO_W: Width is calculated, but height is fixed at 50pt */
  TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 0, 200, 480-2*10), cig_current()->rect);
  cig_pop_frame();
  
  cig_push_frame(RECT_AUTO_W(100));
  TEST_ASSERT_EQUAL_RECT(cig_r_make(200, 0, 100, 480-2*10), cig_current()->rect);
  cig_pop_frame();
  
  TEST_ASSERT_EQUAL_INT(300, cig_current()->_layout_params._h_pos);
  
  cig_pop_frame(); /* Not really necessary in testing, but.. */
}

/*
  In this example we are configuring a h-stack that divides content into 4 columns,
  meaning the RECT_AUTO width would yield "width / 4", but you can still insert an
  explictly sized element as well.
*/
TEST(core_layout, hstack_mix) {
  if (!cig_push_layout_function(&cig_default_layout_builder, RECT_AUTO, cig_i_zero(), (cig_params) {
    .axis = CIG_LAYOUT_AXIS_HORIZONTAL,
    .columns = 4
  })) {
    TEST_FAIL_MESSAGE("Unable to add layout builder frame");
  }

  if (cig_push_frame(RECT_AUTO)) {
    TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 0, 160, 480), cig_current()->rect);
    cig_pop_frame();
  }

  if (cig_push_frame(RECT_AUTO_W(100))) {
    TEST_ASSERT_EQUAL_RECT(cig_r_make(160, 0, 100, 480), cig_current()->rect);
    cig_pop_frame();
  }

  if (cig_push_frame(RECT_AUTO)) {
    TEST_ASSERT_EQUAL_RECT(cig_r_make(260, 0, 160, 480), cig_current()->rect);
    cig_pop_frame();
  }

  if (cig_push_frame(RECT_AUTO)) {
    TEST_ASSERT_EQUAL_RECT(cig_r_make(420, 0, 160, 480), cig_current()->rect);
    cig_pop_frame();
  }

  cig_pop_frame();
}

/*
  V-stack supports bottom-to-top direction as well. Here we're inserting a couple of elements
  with fixed 50pt height, then disable that and add a third item to fill the remaining space.
*/
TEST(core_layout, vstack_align_bottom) {
  register int i;
  if (!cig_push_layout_function(&cig_default_layout_builder, RECT_AUTO, cig_i_zero(), (cig_params) {
    .axis = CIG_LAYOUT_AXIS_VERTICAL,
    .alignment.vertical = CIG_LAYOUT_ALIGNS_BOTTOM,
    .height = 50,
    .spacing = 0
  })) {
    TEST_FAIL_MESSAGE("Unable to add layout builder frame");
  }

  for (i = 0; i < 2; ++i) {
    if (cig_push_frame(RECT_AUTO)) {
      TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 480-50-(i*50), 640, 50), cig_current()->rect);
      cig_pop_frame();
    }
  }

  cig_current()->_layout_params.height = 0;

  cig_push_frame(RECT_AUTO);

  TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 0, 640, 380), cig_current()->rect);
}

/* Similarly, h-stack supports right-to-left alignment */
TEST(core_layout, hstack_align_right) {
  register int i;
  if (!cig_push_layout_function(&cig_default_layout_builder, RECT_AUTO, cig_i_zero(), (cig_params) {
    .axis = CIG_LAYOUT_AXIS_HORIZONTAL,
    .alignment.horizontal = CIG_LAYOUT_ALIGNS_RIGHT,
    .width = 50,
    .spacing = 0
  })) {
    TEST_FAIL_MESSAGE("Unable to add layout builder frame");
  }

  for (i = 0; i < 8; ++i) {
    if (cig_push_frame(RECT_AUTO)) {
      TEST_ASSERT_EQUAL_RECT(cig_r_make(640-50-(i*50), 0, 50, 480), cig_current()->rect);
      cig_pop_frame();
    }
  }
}

TEST(core_layout, standard_frame_alignment) {
  /*  Alignment options can be used for standard frames as well */
  if (!cig_push_layout_function(&cig_default_layout_builder, RECT_AUTO, cig_i_zero(), (cig_params) {
    .alignment.horizontal = CIG_LAYOUT_ALIGNS_RIGHT,
    .alignment.vertical = CIG_LAYOUT_ALIGNS_BOTTOM
  })) {
    TEST_FAIL_MESSAGE("Unable to add layout builder frame");
  }

  /*  Bottom-right alignment means (0,0) is at the bottom-right corner of the parent rectangle */
  cig_push_frame(cig_r_make(0, 0, 200, 100));
  TEST_ASSERT_EQUAL_RECT(cig_r_make(440, 380, 200, 100), cig_current()->rect);
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
  if (!cig_push_grid(RECT_AUTO, cig_i_zero(), (cig_params) {
    .spacing = 0,
    .columns = 5,
    .rows = 5
  })) {
    TEST_FAIL_MESSAGE("Unable to add layout builder frame");
  }

  TEST_ASSERT_FALSE(cig_is_vertical_layout());
  
  int i;
  for (i = 0; i < 25; ++i) {
    int row = (i / 5);
    int column = i - (row * 5);
    cig_push_frame(RECT_AUTO);
    TEST_ASSERT_EQUAL_RECT(cig_r_make(128*column, 96*row, 128, 96), cig_current()->rect);
    cig_pop_frame();
  }

  TEST_ASSERT_EQUAL_INT(0, cig_current()->_layout_params._h_pos);
  TEST_ASSERT_EQUAL_INT(480, cig_current()->_layout_params._v_pos);
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
  if (!cig_push_grid(RECT_AUTO, cig_i_zero(), (cig_params) {
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
  
  TEST_ASSERT_EQUAL_INT(600, cig_current()->_layout_params._h_pos);
  
  /* Second row */
  for (i = 0; i < 3; ++i) {
    cig_push_frame(RECT_AUTO);
    TEST_ASSERT_EQUAL_INT(200, cig_current()->rect.y);
    cig_pop_frame();
  }
  
  TEST_ASSERT_EQUAL_INT(200, cig_current()->_layout_params._v_pos);
}

TEST(core_layout, grid_with_varying_cell_size) {
  /* Third option is to specify a size for each of the cells at insertion time.
     Then, again depending on the remaining space, cell will be inserted into the
     current row or pushed to the next. In addition, you can still specify the number
     of rows and columns, and these will now be used to limit number of cells on each axis */
  if (!cig_push_grid(RECT_AUTO, cig_i_zero(), (cig_params) {
    .limit.horizontal = 3
  })) {
    TEST_FAIL_MESSAGE("Unable to add layout builder frame");
  }

  /* Add 3 elements with increasing width. They should fit on the first row */
  cig_push_frame(RECT_SIZED(100, 160)); /* (1) */
  TEST_ASSERT_EQUAL_INT(0, cig_current()->rect.x);
  cig_pop_frame();
  
  TEST_ASSERT_EQUAL_INT(100, cig_current()->_layout_params._h_pos);
  
  cig_push_frame(RECT_SIZED(200, 160)); /* (2) */
  TEST_ASSERT_EQUAL_INT(100, cig_current()->rect.x);
  cig_pop_frame();
  
  TEST_ASSERT_EQUAL_INT(300, cig_current()->_layout_params._h_pos);
  
  cig_push_frame(RECT_SIZED(300, 160)); /* (3) */
  TEST_ASSERT_EQUAL_INT(300, cig_current()->rect.x);
  cig_pop_frame();
  
  TEST_ASSERT_EQUAL_INT(600, cig_current()->_layout_params._h_pos);
  
  /* Let's try to insert another cell that should fit width wise,
  but the grid would exceed the number of horizontal elements,
  so it's pushed onto the next row */
  cig_push_frame(RECT_SIZED(40, 160)); /* (4) */
  TEST_ASSERT_EQUAL_INT(0,    cig_current()->rect.x);
  TEST_ASSERT_EQUAL_INT(160,  cig_current()->rect.y);
  cig_pop_frame();
  
  /* This one should be inserted normally onto the second row */
  cig_push_frame(RECT_SIZED(400, 160)); /* (5) */
  TEST_ASSERT_EQUAL_INT(40,   cig_current()->rect.x);
  TEST_ASSERT_EQUAL_INT(160,  cig_current()->rect.y);
  cig_pop_frame();
  
  /* Spacer fills the remaining space on the second row. Internally this is just a frame push + pop */
  cig_spacer(CIG__AUTO_BIT);
  
  /* Insert an element to fill all the space on the third row */
  cig_push_frame(RECT_AUTO); /* (6) */
  TEST_ASSERT_EQUAL_INT(640,  cig_current()->rect.w);
  TEST_ASSERT_EQUAL_INT(160,  cig_current()->rect.h);
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
  if (!cig_push_grid(RECT_AUTO, cig_i_zero(), (cig_params) {
    .direction = CIG_LAYOUT_DIRECTION_VERTICAL
  })) {
    TEST_FAIL_MESSAGE("Unable to add layout builder frame");
  }
  
  TEST_ASSERT_TRUE(cig_is_vertical_layout());

  /* Add 3 elements with increasing height and width. They should fit in the first column */
  cig_push_frame(RECT_SIZED(100, 120)); /* (1) */
  TEST_ASSERT_EQUAL_INT(0,    cig_current()->rect.y);
  cig_pop_frame();
  
  TEST_ASSERT_EQUAL_INT(120,  cig_current()->_layout_params._v_pos);
  
  cig_push_frame(RECT_SIZED(150, 160)); /* (2) */
  TEST_ASSERT_EQUAL_INT(120,  cig_current()->rect.y);
  cig_pop_frame();
  
  TEST_ASSERT_EQUAL_INT(280,  cig_current()->_layout_params._v_pos);
  
  cig_push_frame(RECT_SIZED(200, 200)); /* (3) */
  TEST_ASSERT_EQUAL_INT(280,  cig_current()->rect.y);
  cig_pop_frame();
  
  /* No remaining space vertically - position moves back to the top and to the next column */
  TEST_ASSERT_EQUAL_INT(200,  cig_current()->_layout_params._h_pos);
  TEST_ASSERT_EQUAL_INT(0,    cig_current()->_layout_params._v_pos);
  
  /* Without anything else configured on the grid, the next element will fill
     the remaining space on the right */
  cig_push_frame(RECT_AUTO); /* (4) */
  TEST_ASSERT_EQUAL_RECT(cig_r_make(200, 0, 440, 480), cig_current()->rect);
  cig_pop_frame();
  
  TEST_ASSERT_EQUAL_INT(640,  cig_current()->_layout_params._h_pos);
  
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
  if (!cig_push_grid(RECT_AUTO, cig_i_zero(), (cig_params) {
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
    TEST_ASSERT_EQUAL_RECT(cig_r_make(640-200*1, 480-200*1, 200, 200), cig_current()->rect);
    cig_pop_frame(); 
  }

  if (cig_push_frame(RECT_AUTO)) { /* 2 */
    TEST_ASSERT_EQUAL_RECT(cig_r_make(640-200*1, 480-200*2, 200, 200), cig_current()->rect);
    cig_pop_frame(); 
  }

  /*  First (rightmost) column can't fit another 200H element,
      so it moves onto the next column to the left */
  if (cig_push_frame(RECT_AUTO)) { /* 3 */
    TEST_ASSERT_EQUAL_RECT(cig_r_make(640-200*2, 480-200*1, 200, 200), cig_current()->rect);
    cig_pop_frame(); 
  }

  /*  For visualisation:
      ┌────────────────────────────┐
      │            ........xxxxxxxx│
      │            ┌      ┐┌──────┐│
      │             (next) │ 2    ││
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
  register int i;
  /* Any element can be made scrollable, but it makes most sense for stacks/grids */
  if (!cig_push_layout_function(&cig_default_layout_builder, RECT_AUTO, cig_i_zero(), (cig_params) {
    .axis = CIG_LAYOUT_AXIS_VERTICAL,
    .height = 100
  })) {
    TEST_FAIL_MESSAGE("Unable to add layout builder frame");
  }

  /*  Scrolling is not enabled by default */
  TEST_ASSERT_NULL(cig_current()->_scroll_state);
  TEST_ASSERT_FALSE(cig_current()->_flags & CLIPPED);

  cig_enable_scroll(NULL);

  /* Scrolling also enables clipping */
  TEST_ASSERT_TRUE(cig_current()->_flags & CLIPPED);

  cig_scroll_state_t *scroll = cig_scroll_state();

  TEST_ASSERT_EQUAL_VEC2(cig_v_zero(), scroll->offset);
  TEST_ASSERT_EQUAL_RECT(cig_r_zero(), cig_content_rect());

  scroll->offset.y = 220;

  /*  Let's add some content to the stack */
  for (i = 0; i < 10; ++i) {
    if (cig_push_frame(RECT_AUTO)) {
      /* Elements should be offset by scroll amount on the Y axis */
      TEST_ASSERT_EQUAL_INT(i*100-220, cig_current()->rect.y);
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
  TEST_ASSERT_EQUAL_RECT(cig_r_make(100, 0, 440, 100), cig_current()->clipped_rect);
  cig_pop_frame();
  
  /*  Some element filling the whole root */
  cig_push_frame(RECT_AUTO);

  /*  Clipping is now turned ON for the current layout element */
  cig_enable_clipping();

  cig_push_frame(cig_r_make(600, 400, 100, 100));

  TEST_ASSERT_EQUAL_RECT(cig_r_make(600, 400, 40, 80), cig_current()->clipped_rect);
    
    cig_push_frame(cig_r_make(30, 70, 20, 20));
    TEST_ASSERT_EQUAL_RECT(cig_r_make(30, 70, 10, 10), cig_current()->clipped_rect);
    cig_pop_frame();
    
  cig_pop_frame();

  cig_push_frame(cig_r_make(-75, -50, 200, 200));
  
  TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 0, 125, 150), cig_current()->clipped_rect);
    
    cig_push_frame(cig_r_make(0, 0, 100, 100));
    TEST_ASSERT_EQUAL_RECT(cig_r_make(75, 50, 25, 50), cig_current()->clipped_rect);
    cig_pop_frame();
    
    cig_push_frame(cig_r_make(-25, -50, 100, 100));
    TEST_ASSERT_EQUAL_RECT(cig_r_make(75, 50, 0, 0), cig_current()->clipped_rect);
    cig_pop_frame();

  cig_pop_frame();

  cig_push_frame(cig_r_make(100, 100, 440, 280));
  cig_enable_clipping();

    cig_push_frame(cig_r_make(-10, -50, 460, 100));
    TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 0, 440, 50), cig_current()->clipped_rect);
    cig_pop_frame();

  cig_pop_frame();

  cig_pop_frame();
}

TEST(core_layout, additional_buffers) {
  int secondary_buffer = 2;

  cig_push_frame(cig_r_make(100, 100, 440, 280));
  cig_push_buffer(&secondary_buffer);

  TEST_ASSERT_EQUAL(&secondary_buffer, cig_buffer());
  TEST_ASSERT_EQUAL_RECT(cig_r_make(100, 100, 440, 280), cig_current()->clipped_rect);
  TEST_ASSERT_EQUAL_RECT(cig_r_make(100, 100, 440, 280), cig_current()->absolute_rect);

  /*  Another child within that new buffer */
  cig_push_frame(cig_r_make(100, 100, 240, 80));

  TEST_ASSERT_EQUAL_RECT(cig_r_make(100, 100, 240, 80), cig_current()->rect);

  /*  Absolute frame is now in the coordinate space of the new buffer, rather than the main buffer */
  TEST_ASSERT_EQUAL_RECT(cig_r_make(100, 100, 240, 80), cig_current()->absolute_rect);

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
  TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 240, 640, 240), cig_current()->clipped_rect);

  cig_push_frame(RECT_AUTO);

  /*  The absolute frame should be offset as well, while the relative frames stay the same */
  TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 0, 640, 240), cig_current()->rect);
  TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 0, 640, 240), cig_current()->clipped_rect);
  TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 240, 640, 240), cig_current()->absolute_rect);

  cig_pop_frame();
}

TEST(core_layout, relative_values) {
  /*  CIG_REL allows us to create frames with relative sizes */
  if (cig_push_frame(RECT(0, 0, CIG_REL(0.5), CIG_REL(0.75)))) {
    TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 0, 320, 360), cig_current()->rect);
    cig_pop_frame();
  }

  /*  The same works for X and Y */
  if (cig_push_frame(RECT(CIG_REL(-0.1), CIG_REL(0.5), 100, 100))) {
    TEST_ASSERT_EQUAL_RECT(cig_r_make(-64, 240, 100, 100), cig_current()->rect);
    cig_pop_frame();
  } else {
    TEST_FAIL_MESSAGE("Frame not added");
  }

  /*  CIG_REL and CIG_AUTO can also work together. In standard frames
      CIG_AUTO(CIG_REL(0.5)) is equivalent to CIG_REL(0.5),
      but in stacks for example it can yield different values */
  if (cig_push_vstack(RECT_AUTO, cig_i_zero(), (cig_params) {
    .height = 70
  })) {
    /*  This stack has 70px rows by default but we can specify we want DOUBLE that.
        This isn't very intuitive though? */
    if (cig_push_frame(RECT(0, 0, CIG_AUTO(), CIG_AUTO(CIG_REL(2.0))))) {
      TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 0, 640, 140), cig_current()->rect);
      cig_pop_frame();
    }
  }
}

TEST(core_layout, pinning) {
  /*  Pinning is an alternative way of constructing the layout rectangle
      by referencing existing elements for positioning or dimensioning.
      One may do all these calculations manually as well, but using
      a builder function simplifies it a lot IMO.

      (1) This should not be used to insert frames into stacks and grids
      as those have their own layout logic and would override values provided.

      (2) Elements being referenced must be an ancestor or a descendant in the
      currently open element. */

  cig_frame *root = cig_current();

  /*  The following element is 8px from left and 10px from top edge of 'root',
      and has an explicit width of 70px and height of 50px. */
  cig_frame *f0 = cig_retain(cig_push_frame(cig_build_rect(4, (cig_pin[]) {
    { LEFT, 8, root, LEFT },
    { TOP, 10, root, TOP },
    { WIDTH, 70 },
    { HEIGHT, 50 }
  })));
  TEST_ASSERT_EQUAL_RECT(cig_r_make(8, 10, 70, 50), f0->rect);
  cig_pop_frame();


  /*  The second element is below and after 'f0' but 10px larger on both axis. */
  cig_frame *f1 = cig_retain(cig_push_frame(cig_build_rect(4, (cig_pin[]) {
    { LEFT, 0, f0, RIGHT },
    { TOP, 0, f0, BOTTOM },
    { WIDTH, 10, f0 },
    { HEIGHT, 10, f0 }
  })));
  TEST_ASSERT_EQUAL_RECT(cig_r_make(78, 60, 80, 60), f1->rect);
  cig_pop_frame();


  /* What we should have at this point:
     ┌─────────────────────────────────┐
     │┌f0──┐                           │
     ││    │                           │
     │└────+f1────┐                    │
     │     │      │                    │
     │     │      │                    │
     │     └──────┘                    │
     │                                 │
     │                                 │
     │                                 │
     │                                 │
     └─────────────────────────────────┘

    Let's continue so that we would end up with something like this:
     ┌─────────────────────────────────┐
     │┌f0──┐      ╔[f2]════════════════╡
     ││    │      ║                    │
     │└────+f1────╢                    │
     │     │      ║                    │
     │     │      ║                    │
     │     └──────╢                    │
     │      ↓10px ╚════════════════════╡
     │                                 │
     │                                 │
     │                                 │
     └─────────────────────────────────┘ */

  cig_frame *f2 = cig_retain(cig_push_frame(cig_build_rect(4, (cig_pin[]) {
    { LEFT, 0, f1, RIGHT },
    { RIGHT, 0, root, RIGHT },
    { TOP, 0, f0, TOP },
    { BOTTOM, -10, f1, BOTTOM },
  })));

  TEST_ASSERT_EQUAL_RECT(cig_r_make(158, 10, 482, 120), f2->rect);


  /*  Now we'll center something inside 'f2' like so:
     ┌─────────────────────────────────┐
     │┌f0──┐      ┌f2───────:──────────┤
     ││    │      │         :          │
     │└────+f1────┤      ╔[f3]══╗      │
     │     │      │......║      ║......│
     │     │      │      ╚══════╝      │
     │     └──────┤         :          │
     │            └─────────:──────────┤
     │                                 │
     │                                 │
     │                                 │
     └─────────────────────────────────┘ */

  cig_frame *f3 = cig_retain(cig_push_frame(cig_build_rect(4, (cig_pin[]) {
    { CENTER_X, 0, f2 },
    { CENTER_Y, 0, f2 },
    { WIDTH, 50 },
    { HEIGHT, 50 },
  })));
  TEST_ASSERT_EQUAL_RECT(cig_r_make(216, 35, 50, 50), f3->rect);
  cig_pop_frame();

  cig_pop_frame();


  /*  Now even though 'f3' is inside f2, we can still reference it outside
      when positioning our next element, like so:

      ┌─────────────────────────────────┐
      │┌f0──┐      ┌f2──────────────────┤
      ││    │      │                    │
      │└────+f1────┤      ┌f3────┐      │
      │     │      │      │      │      │
      │     │   :  │      └──────┘      │
      │     └───:──┤             :      │
      │         ╔[f4]════════════╦──────┤
      │         ║                ║      │
      │         ║                ║      │
      │         ║                ║      │
      └─────────╨────────────────╨──────┘

      Left side will align with center X of 'f1', right will align with right side
      of 'f3', top edge will be just below f2 and bottom edge will align with root. */
  
  cig_frame *f4 = cig_retain(cig_push_frame(cig_build_rect(4, (cig_pin[]) {
    { LEFT, 0, f1, CENTER_X },
    { RIGHT, 0, f3, RIGHT },
    { TOP, 0, f2, BOTTOM },
    { BOTTOM, 0, root, BOTTOM }
  })));
  TEST_ASSERT_EQUAL_RECT(cig_r_make(118, 130, 306, 350), f4->rect);
  cig_pop_frame();
}

TEST(core_layout, pinning_with_insets) {
  cig_frame *root = cig_current();

  root->insets = cig_i_uniform(10);


  cig_frame *f0 = cig_retain(cig_push_frame_insets(cig_build_rect(4, (cig_pin[]) {
    { LEFT, 0, root, LEFT | INSET_ATTRIBUTE },
    { TOP, 0, root, TOP | INSET_ATTRIBUTE },
    { WIDTH, 100 },
    { HEIGHT, 100 }
  }), cig_i_uniform(10)));
  TEST_ASSERT_EQUAL_RECT(cig_r_make(10, 10, 100, 100), f0->absolute_rect);
  cig_pop_frame();


  /* 
      ┌─────────────────────────────────┐
      │ .....10......                   │
      │.┌f0─────────┐                   │
      │.│  ┄┄┄10┄┄  │                   │
      │1│1┆       ┆ │                   │
      │0│0┆       ┆ │                   │
      │.│  ┄┄┄┄┄┄┄  │                   │
      │.└───────────┘                   │

      Next we want to align another rectangle using the insets of 'f0', like so:
  
      ┌─────────────────────────────────┐
      │                                 │
      │ ┌f0─────────┐                   │
      │ │  ┄┄┄┄┄┄┄  │                   │
      │ │ ┆       ┆ │                   │
      │ │ ┆       ┆ │                   │
      │ │ ╔[f1]═════╗                   │
      │ └─║         ║                   │
      │   ║         ║                   │
      │   ║         ║                   │
      │   ╚═════════╝                   │
      └─────────────────────────────────┘ */

  cig_frame *f1 = cig_retain(cig_push_frame(cig_build_rect(4, (cig_pin[]) {
    { LEFT, 0, f0, LEFT | INSET_ATTRIBUTE },
    { TOP, 0, f0, BOTTOM | INSET_ATTRIBUTE },
    { RIGHT, 0, f0, RIGHT },
    { HEIGHT, 100 }
  })));
  TEST_ASSERT_EQUAL_RECT(cig_r_make(20, 100, 100, 100), f1->absolute_rect);
  cig_pop_frame();
}

TEST(core_layout, pinning_relative) {
  cig_frame *root = cig_current();


  /*  Center and size using relative values */
  cig_frame *first = cig_push_frame(cig_build_rect(4, (cig_pin[]) {
    { CENTER_X, CIG_REL(-0.25), root },
    { CENTER_Y, CIG_REL(0.25), root },
    { WIDTH, CIG_REL(0.4), root },
    { HEIGHT, CIG_REL(0.6), root }
  }));

  TEST_ASSERT_EQUAL_RECT(cig_r_make(32, 216, 256, 288), first->absolute_rect);

  cig_pop_frame();


  /* Testing relative left, right, top and bottom also */
  cig_frame *second = cig_push_frame(cig_build_rect(4, (cig_pin[]) {
    { LEFT, CIG_REL(0.2), root },
    { RIGHT, CIG_REL(0.2), root },
    { TOP, CIG_REL(0.1), root },
    { BOTTOM, CIG_REL(0.1), root }
  }));

  TEST_ASSERT_EQUAL_RECT(cig_r_make(128, 48, 384, 384), second->absolute_rect);

  cig_pop_frame();
}

TEST(core_layout, pinning_infer_edges) {
  cig_frame *root = cig_current();

  /*  Left and top edges will be inferred by using center and right and bottom edges respectively */
  cig_frame *f0 = cig_push_frame(cig_build_rect(4, (cig_pin[]) {
    { CENTER_X, 0, root },
    { RIGHT, 50, root },
    { CENTER_Y, 0, root },
    { BOTTOM, 50, root }
  }));
  TEST_ASSERT_EQUAL_RECT(cig_r_make(50, 50, 540, 380), f0->absolute_rect);
  cig_pop_frame();


  /*  If all else fails, left and top will default to zero as long as width/height is there */
  cig_frame *f1 = cig_push_frame(cig_build_rect(2, (cig_pin[]) {
    { WIDTH, 100 },
    { HEIGHT, 75 },
  }));
  TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 0, 100, 75), f1->absolute_rect);
  cig_pop_frame();
}

TEST(core_layout, pinning_aspect_ratio) {
  cig_frame *root = cig_current();

  cig_r r0 = cig_build_rect(4, (cig_pin[]) {
    { CENTER_X, 0, root },
    { CENTER_Y, 0, root },
    { WIDTH, 400 },
    { ASPECT, 4/3.0 }
  });

  TEST_ASSERT_EQUAL_RECT(cig_r_make(120, 90, 400, 300), r0);


  cig_r r1 = cig_build_rect(4, (cig_pin[]) {
    { CENTER_X, 0, root },
    { CENTER_Y, 0, root },
    { HEIGHT, 300 },
    { ASPECT, 4/3.0 }
  });

  TEST_ASSERT_EQUAL_RECT(cig_r_make(120, 90, 400, 300), r1);
}

TEST(core_layout, pinning_attribute_sign_swap) {
  cig_frame *root = cig_current();

  cig_r r0 = cig_build_rect(4, (cig_pin[]) {
    { BOTTOM, 50, root }, /* Bottom edge is set 50pt above the bottom edge of 'root' */
    { RIGHT, 50, root }, /* Right edge is set 50pt left of right edge of 'root' */
    { WIDTH, 100 },
    { HEIGHT, 100 },
  });

  TEST_ASSERT_EQUAL_RECT(cig_r_make(490, 330, 100, 100), r0);


  cig_r r1 = cig_build_rect(4, (cig_pin[]) {
    { TOP, 50, root, BOTTOM }, /* Top edge is set 50pt below the bottom edge of 'root' */
    { LEFT, 50, root, RIGHT }, /* Left edge is set 50pt right of right edge of 'root' */
    { WIDTH, 100 },
    { HEIGHT, 100 },
  });

  TEST_ASSERT_EQUAL_RECT(cig_r_make(690, 530, 100, 100), r1);
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
  RUN_TEST_CASE(core_layout, pinning)
  RUN_TEST_CASE(core_layout, pinning_with_insets)
  RUN_TEST_CASE(core_layout, pinning_relative)
  RUN_TEST_CASE(core_layout, pinning_infer_edges)
  RUN_TEST_CASE(core_layout, pinning_aspect_ratio)
  RUN_TEST_CASE(core_layout, pinning_attribute_sign_swap)
}
