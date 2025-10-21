#include "unity.h"
#include "fixture.h"
#include "cigcore.h"
#include "asserts.h"

#define FRAME_TIME 0.1f

TEST_GROUP(core_input);

static cig_context ctx = { 0 };

TEST_SETUP(core_input) {
  cig_init_context(&ctx);
}
TEST_TEAR_DOWN(core_input) {}

static void begin(float frame_time) {
  cig_begin_layout(&ctx, NULL, cig_r_make(0, 0, 640, 480), frame_time);
}

static void end() {
  cig_end_layout();
}

/*  ┌────────────┐
    │ TEST CASES │
    └────────────┘ */

TEST(core_input, hover_and_press) {
  register int i;
  for (i = 0; i < 2; ++i) {
    begin(FRAME_TIME);
    cig_set_input_state(cig_v_make(50, 50), i == 1 ? CIG_INPUT_PRIMARY_ACTION : 0);
    cig_push_frame(cig_r_make(0, 0, 100, 100));
    cig_enable_interaction(); /* This element now tracks mouse inputs */

    if (i == 1) {
      TEST_ASSERT_TRUE(cig_hovered());
      TEST_ASSERT_TRUE(cig_pressed(CIG_INPUT_ACTION_ANY, CIG_PRESS_INSIDE));
    }

    cig_pop_frame();
    end();
  }
}

TEST(core_input, overlapping_hover_and_press) {
  register int i;
  for (i = 0; i < 2; ++i) {
    begin(FRAME_TIME);
    cig_set_input_state(cig_v_make(75, 75), i == 1 ? CIG_INPUT_PRIMARY_ACTION : 0);
    cig_push_frame(cig_r_make(0, 0, 100, 100));
    cig_enable_interaction(); /* This element now tracks mouse inputs */

    if (i == 1) {
      TEST_ASSERT_FALSE(cig_hovered());
      TEST_ASSERT_FALSE(cig_pressed(CIG_INPUT_ACTION_ANY, 0));
    }

    cig_pop_frame();
    cig_push_frame(cig_r_make(50, 50, 100, 100));
    cig_enable_interaction();

    if (i == 1) {
      TEST_ASSERT_TRUE(cig_hovered());
      TEST_ASSERT_TRUE(cig_pressed(CIG_INPUT_ACTION_ANY, 0));
    }

    /*  Even if there's an additional element on top of the current one,
        unless we call `im_enable_interaction`, that element is not included in mouse detection */
    cig_push_frame(RECT_AUTO);
    cig_pop_frame();
    cig_pop_frame();
    end();
  }
}

TEST(core_input, click_on_release) {
  register int i;
  for (i = 0; i < 3; ++i) {
    begin(FRAME_TIME);
    cig_set_input_state(cig_v_make(75, 75), i == 1 ? CIG_INPUT_PRIMARY_ACTION : 0);
    cig_push_frame(cig_r_make(0, 0, 100, 100));
    cig_pop_frame();
    cig_push_frame(cig_r_make(50, 50, 100, 100));
    cig_enable_interaction();

    if (i == 2) {
      TEST_ASSERT_TRUE(cig_clicked(CIG_INPUT_ACTION_ANY, CIG_CLICK_STARTS_INSIDE));
    }

    cig_pop_frame();
    end();
  }
}

TEST(core_input, click_on_button_down) {
  register int i;
  for (i = 0; i < 2; ++i) {
    begin(FRAME_TIME);
    cig_set_input_state(cig_v_make(75, 75), i == 1 ? CIG_INPUT_PRIMARY_ACTION : 0);
    cig_push_frame(cig_r_make(50, 50, 100, 100));
    cig_enable_interaction();

    if (i == 1) {
      /*  Click is detected as soon as mouse button is pressed */
      TEST_ASSERT_TRUE(cig_clicked(CIG_INPUT_ACTION_ANY, CIG_CLICK_ON_PRESS));
    }

    cig_pop_frame();
    end();
  }
}

TEST(core_input, click_starts_outside) {
  register int i;
  for (i = 0; i < 3; ++i) {
    begin(FRAME_TIME);

    /*  Simulate mouse change over time */
    if (i == 0) {
      cig_set_input_state(cig_v_make(25, 25), 0);
    } else if (i == 1) {
      cig_set_input_state(cig_v_make(75, 75), CIG_INPUT_PRIMARY_ACTION);
    } else if (i == 2) {
      cig_set_input_state(cig_v_make(75, 75), 0);
    }

    cig_push_frame(cig_r_make(50, 50, 100, 100));
    cig_enable_interaction();

    if (i == 2) {
      /*  Mouse was clicked when outside of this element, so even when moving
          the cursor over when releasing the button, click is not tracked */
      TEST_ASSERT_FALSE(cig_clicked(CIG_INPUT_ACTION_ANY, CIG_CLICK_STARTS_INSIDE));
    }

    cig_pop_frame();
    end();
  }
}

TEST(core_input, double_click) {
  register int i;
  for (i = 0; i < 5; ++i) {
    begin(FRAME_TIME);
    cig_set_input_state(cig_v_make(75, 75), i == 1 || i == 3 ? CIG_INPUT_PRIMARY_ACTION : 0);
    cig_push_frame(cig_r_make(50, 50, 100, 100));
    cig_enable_interaction();

    if (i == 4) {
      TEST_ASSERT_TRUE(cig_clicked(CIG_INPUT_ACTION_ANY, CIG_CLICK_DOUBLE));
    }

    cig_pop_frame();
    end();
  }
}

TEST(core_input, double_click_too_slow) {
  register int i;
  /*  Default maximum time between clicks is 0.5 seconds */
  for (i = 0; i < 5; ++i) {
    begin(FRAME_TIME * 10);
    cig_set_input_state(cig_v_make(75, 75), i == 1 || i == 3 ? CIG_INPUT_PRIMARY_ACTION : 0);
    cig_push_frame(cig_r_make(50, 50, 100, 100));
    cig_enable_interaction();

    if (i == 4) {
      TEST_ASSERT_FALSE(cig_clicked(CIG_INPUT_ACTION_ANY, CIG_CLICK_DOUBLE));
    }

    cig_pop_frame();
    end();
  }
}

TEST(core_input, simple_drag) {
  register int i;
  for (i = 0; i < 8; ++i) {
    begin(FRAME_TIME);

    switch (i) {
    case 0:
      cig_set_input_state(cig_v_make(25, 25), 0);
      break;
    case 1:
      cig_set_input_state(cig_v_make(25, 25), CIG_INPUT_PRIMARY_ACTION);
      break;
    case 2:
    case 3:
      cig_set_input_state(cig_v_make(30, 30), CIG_INPUT_PRIMARY_ACTION);
      break;
    case 4:
      cig_set_input_state(cig_v_make(35, 35), CIG_INPUT_PRIMARY_ACTION);
      break;
    case 5:
    case 6:
    case 7:
      cig_set_input_state(cig_v_make(45, 45), 0);
      break;
    }

    cig_enable_interaction();

    switch (i) {
    case 0: /* Button up */
      TEST_ASSERT_EQUAL(CIG_DRAG_STATE_INACTIVE, cig_dragged(CIG_INPUT_PRIMARY_ACTION));
      TEST_ASSERT_EQUAL(0, cig_input_state()->drag.id);
      TEST_ASSERT_FALSE(cig_input_state()->locked);
      break;
    case 1: /* Button down, no movement */
      TEST_ASSERT_EQUAL(CIG_DRAG_STATE_READY, cig_dragged(CIG_INPUT_PRIMARY_ACTION));
      TEST_ASSERT_EQUAL(cig_current()->id, cig_input_state()->drag.id);
      TEST_ASSERT_EQUAL_VEC2(cig_v_make(25, 25), cig_input_state()->drag._start_position_absolute);
      TEST_ASSERT_EQUAL_VEC2(cig_v_zero(), cig_input_state()->drag.change_total);
      break;
    case 2: /* Initial mouse movement */
      cig_input_state()->locked = true;
      TEST_ASSERT_EQUAL(CIG_DRAG_STATE_BEGAN, cig_dragged(CIG_INPUT_PRIMARY_ACTION));
      TEST_ASSERT_EQUAL_VEC2(cig_v_make(5, 5), cig_input_state()->drag.change_total);
      TEST_ASSERT_EQUAL_VEC2(cig_v_make(5, 5), cig_input_state()->drag.change_last_frame);
      break;
    case 3: /* No mouse movement */
      TEST_ASSERT_EQUAL(CIG_DRAG_STATE_IDLE, cig_dragged(CIG_INPUT_PRIMARY_ACTION));
      TEST_ASSERT_EQUAL_VEC2(cig_v_make(5, 5), cig_input_state()->drag.change_total);
      TEST_ASSERT_EQUAL_VEC2(cig_v_make(0, 0), cig_input_state()->drag.change_last_frame);
      break;
    case 4: /* Mouse moved again */
      TEST_ASSERT_EQUAL(CIG_DRAG_STATE_MOVED, cig_dragged(CIG_INPUT_PRIMARY_ACTION));
      TEST_ASSERT_EQUAL_VEC2(cig_v_make(10, 10), cig_input_state()->drag.change_total);
      TEST_ASSERT_EQUAL_VEC2(cig_v_make(5, 5), cig_input_state()->drag.change_last_frame);
      break;
    case 5: /* Mouse moved and button released */
      /* When button is released, the mouse may still have movsed compared to last frame.
         In that case a final 'MOVED' state is emitted, followed by 'ENDED' */
      TEST_ASSERT_EQUAL(CIG_DRAG_STATE_MOVED, cig_dragged(CIG_INPUT_PRIMARY_ACTION));
      TEST_ASSERT_EQUAL_VEC2(cig_v_make(20, 20), cig_input_state()->drag.change_total);
      TEST_ASSERT_EQUAL_VEC2(cig_v_make(10, 10), cig_input_state()->drag.change_last_frame);
      break;
    case 6: /* Button up */
      TEST_ASSERT_EQUAL(CIG_DRAG_STATE_ENDED, cig_dragged(CIG_INPUT_PRIMARY_ACTION));
      TEST_ASSERT_EQUAL_VEC2(cig_v_make(20, 20), cig_input_state()->drag.change_total);
      TEST_ASSERT_EQUAL_VEC2(cig_v_make(0, 0), cig_input_state()->drag.change_last_frame);
      break;
    case 7: /* Button up */
      TEST_ASSERT_EQUAL(CIG_DRAG_STATE_INACTIVE, cig_dragged(CIG_INPUT_PRIMARY_ACTION));
      TEST_ASSERT_EQUAL(0, cig_input_state()->drag.id);
      TEST_ASSERT_FALSE(cig_input_state()->locked);
      break;
    }

    cig_push_frame(cig_r_make(32, 32, 100, 100));
    cig_enable_interaction();

    if (i == 2) {
      /* Even though the mouse is over this element at this point, lock
       * prevents it from being detected */
      TEST_ASSERT_FALSE(cig_hovered());
    }

    cig_pop_frame();
    end();
  }
}

TEST(core_input, drag_other_input_button) {
register int i;
  for (i = 0; i < 3; ++i) {
    begin(FRAME_TIME);

    switch (i) {
    case 0:
      cig_set_input_state(cig_v_make(25, 25), 0);
      break;
    case 1:
      cig_set_input_state(cig_v_make(30, 30), CIG_INPUT_SECONDARY_ACTION);
      break;
    case 2:
      cig_set_input_state(cig_v_make(35, 35), CIG_INPUT_SECONDARY_ACTION);
      break;
    }

    cig_enable_interaction();

    /* Drag state is not activated because the action type is not recognized */
    TEST_ASSERT_EQUAL(CIG_DRAG_STATE_INACTIVE, cig_dragged(CIG_INPUT_PRIMARY_ACTION));
    TEST_ASSERT_EQUAL(0, cig_input_state()->drag.id);

    end();
  }
}

TEST(core_input, button_states) {
  begin(0);

  /* (Time 0) */
  cig_set_input_state(cig_v_zero(), CIG_INPUT_PRIMARY_ACTION);

  TEST_ASSERT_TRUE(cig_input_state()->action_mask & CIG_INPUT_PRIMARY_ACTION);
  TEST_ASSERT_EQUAL(CIG_INPUT_PRIMARY_ACTION, cig_input_state()->last_action_began);
  TEST_ASSERT_EQUAL(0, cig_input_state()->last_action_ended);
  TEST_ASSERT_EQUAL(BEGAN, cig_input_state()->click_state);

  /* (T1) */
  cig_set_input_state(cig_v_zero(), CIG_INPUT_PRIMARY_ACTION | CIG_INPUT_SECONDARY_ACTION);

  TEST_ASSERT_TRUE(cig_input_state()->action_mask & CIG_INPUT_ACTION_ANY);
  TEST_ASSERT_EQUAL(CIG_INPUT_SECONDARY_ACTION, cig_input_state()->last_action_began);
  TEST_ASSERT_EQUAL(0, cig_input_state()->last_action_ended);
  /* Maybe this should report a failed case or something because you're not
     pressing down both mouse buttons and expecting a click event normally? */
  TEST_ASSERT_EQUAL(NEITHER, cig_input_state()->click_state);

  /* (T2) */
  cig_set_input_state(cig_v_zero(), CIG_INPUT_SECONDARY_ACTION);
  
  TEST_ASSERT_TRUE(cig_input_state()->action_mask & CIG_INPUT_SECONDARY_ACTION);
  TEST_ASSERT_EQUAL(CIG_INPUT_PRIMARY_ACTION, cig_input_state()->last_action_ended);
  TEST_ASSERT_EQUAL(NEITHER, cig_input_state()->click_state);

  /* (T3) */
  cig_set_input_state(cig_v_zero(), 0);
  
  TEST_ASSERT_EQUAL(0, cig_input_state()->action_mask);
  TEST_ASSERT_EQUAL(CIG_INPUT_SECONDARY_ACTION, cig_input_state()->last_action_ended);
  TEST_ASSERT_EQUAL(ENDED, cig_input_state()->click_state);

  end();
}

TEST_GROUP_RUNNER(core_input) {
  RUN_TEST_CASE(core_input, hover_and_press);
  RUN_TEST_CASE(core_input, overlapping_hover_and_press);
  RUN_TEST_CASE(core_input, click_on_release);
  RUN_TEST_CASE(core_input, click_on_button_down);
  RUN_TEST_CASE(core_input, click_starts_outside);
  RUN_TEST_CASE(core_input, double_click);
  RUN_TEST_CASE(core_input, double_click_too_slow);
  RUN_TEST_CASE(core_input, simple_drag);
  RUN_TEST_CASE(core_input, drag_other_input_button);
  RUN_TEST_CASE(core_input, button_states);
}
