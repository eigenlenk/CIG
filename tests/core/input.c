#include "unity.h"
#include "fixture.h"
#include "cigcore.h"
#include "asserts.h"

#define FRAME_TIME 0.1f

#define TEST_CYCLE(BODY) \
  begin(FRAME_TIME); \
  BODY; \
  end();

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
    cig_set_pointer_position(cig_v_make(50, 50));
    cig_set_pointer_state(i == 1 ? CIG_INPUT_PRIMARY_ACTION : 0);
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
    cig_set_pointer_position(cig_v_make(75, 75));
    cig_set_pointer_state(i == 1 ? CIG_INPUT_PRIMARY_ACTION : 0);
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
    cig_set_pointer_position(cig_v_make(75, 75));
    cig_set_pointer_state(i == 1 ? CIG_INPUT_PRIMARY_ACTION : 0);
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
    cig_set_pointer_position(cig_v_make(75, 75));
    cig_set_pointer_state(i == 1 ? CIG_INPUT_PRIMARY_ACTION : 0);
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
      cig_set_pointer_position(cig_v_make(25, 25));
      cig_set_pointer_state(0);
    } else if (i == 1) {
      cig_set_pointer_position(cig_v_make(75, 75));
      cig_set_pointer_state(CIG_INPUT_PRIMARY_ACTION);
    } else if (i == 2) {
      cig_set_pointer_position(cig_v_make(75, 75));
      cig_set_pointer_state(0);
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
    cig_set_pointer_position(cig_v_make(75, 75));
    cig_set_pointer_state(i == 1 || i == 3 ? CIG_INPUT_PRIMARY_ACTION : 0);
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
    cig_set_pointer_position(cig_v_make(75, 75));
    cig_set_pointer_state(i == 1 || i == 3 ? CIG_INPUT_PRIMARY_ACTION : 0);
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
  for (i = 0; i < 9; ++i) {
    begin(FRAME_TIME);

    switch (i) {
    case 0:
      cig_set_pointer_position(cig_v_make(25, 25));
      cig_set_pointer_state(0);
      break;
    case 1:
    case 2:
      cig_set_pointer_position(cig_v_make(25, 25));
      cig_set_pointer_state(CIG_INPUT_PRIMARY_ACTION);
      break;
    case 3:
    case 4:
      cig_set_pointer_position(cig_v_make(30, 30));
      cig_set_pointer_state(CIG_INPUT_PRIMARY_ACTION);
      break;
    case 5:
      cig_set_pointer_position(cig_v_make(35, 35));
      cig_set_pointer_state(CIG_INPUT_PRIMARY_ACTION);
      break;
    case 6:
    case 7:
    case 8:
      cig_set_pointer_position(cig_v_make(45, 45));
      cig_set_pointer_state(0);
      break;
    }

    cig_enable_interaction();

    switch (i) {
    case 0: /* Button up */
      TEST_ASSERT_EQUAL(CIG_DRAG_STATE_INACTIVE, cig_dragged(CIG_INPUT_PRIMARY_ACTION));
      TEST_ASSERT_EQUAL(0, cig_input_state()->pointer.drag.id);
      TEST_ASSERT_FALSE(cig_input_state()->pointer.locked);
      break;
    case 1: /* Button down, no movement */
      TEST_ASSERT_EQUAL(CIG_DRAG_STATE_READY, cig_dragged(CIG_INPUT_PRIMARY_ACTION));
      TEST_ASSERT_EQUAL(cig_current()->id, cig_input_state()->pointer.drag.id);
      TEST_ASSERT_EQUAL_VEC2(cig_v_make(25, 25), cig_input_state()->pointer.drag._start_position_absolute);
      TEST_ASSERT_EQUAL_VEC2(cig_v_zero(), cig_input_state()->pointer.drag.change_total);
      break;
    case 2: /* Button down, still no movement */
      TEST_ASSERT_EQUAL(CIG_DRAG_STATE_WAITING_MOVE, cig_dragged(CIG_INPUT_PRIMARY_ACTION));
      TEST_ASSERT_EQUAL(cig_current()->id, cig_input_state()->pointer.drag.id);
      TEST_ASSERT_EQUAL_VEC2(cig_v_make(25, 25), cig_input_state()->pointer.drag._start_position_absolute);
      TEST_ASSERT_EQUAL_VEC2(cig_v_zero(), cig_input_state()->pointer.drag.change_total);
      break;
    case 3: /* Initial mouse movement */
      cig_input_state()->pointer.locked = true;
      TEST_ASSERT_EQUAL(CIG_DRAG_STATE_BEGAN, cig_dragged(CIG_INPUT_PRIMARY_ACTION));
      TEST_ASSERT_EQUAL_VEC2(cig_v_make(5, 5), cig_input_state()->pointer.drag.change_total);
      TEST_ASSERT_EQUAL_VEC2(cig_v_make(5, 5), cig_input_state()->pointer.drag.change_last_frame);
      break;
    case 4: /* No mouse movement */
      TEST_ASSERT_EQUAL(CIG_DRAG_STATE_IDLE, cig_dragged(CIG_INPUT_PRIMARY_ACTION));
      TEST_ASSERT_EQUAL_VEC2(cig_v_make(5, 5), cig_input_state()->pointer.drag.change_total);
      TEST_ASSERT_EQUAL_VEC2(cig_v_make(0, 0), cig_input_state()->pointer.drag.change_last_frame);
      break;
    case 5: /* Mouse moved again */
      TEST_ASSERT_EQUAL(CIG_DRAG_STATE_MOVED, cig_dragged(CIG_INPUT_PRIMARY_ACTION));
      TEST_ASSERT_EQUAL_VEC2(cig_v_make(10, 10), cig_input_state()->pointer.drag.change_total);
      TEST_ASSERT_EQUAL_VEC2(cig_v_make(5, 5), cig_input_state()->pointer.drag.change_last_frame);
      break;
    case 6: /* Mouse moved and button released */
      /* When button is released, the mouse may still have movsed compared to last frame.
         In that case a final 'MOVED' state is emitted, followed by 'ENDED' */
      TEST_ASSERT_EQUAL(CIG_DRAG_STATE_MOVED, cig_dragged(CIG_INPUT_PRIMARY_ACTION));
      TEST_ASSERT_EQUAL_VEC2(cig_v_make(20, 20), cig_input_state()->pointer.drag.change_total);
      TEST_ASSERT_EQUAL_VEC2(cig_v_make(10, 10), cig_input_state()->pointer.drag.change_last_frame);
      break;
    case 7: /* Button up */
      TEST_ASSERT_EQUAL(CIG_DRAG_STATE_ENDED, cig_dragged(CIG_INPUT_PRIMARY_ACTION));
      TEST_ASSERT_EQUAL_VEC2(cig_v_make(20, 20), cig_input_state()->pointer.drag.change_total);
      TEST_ASSERT_EQUAL_VEC2(cig_v_make(0, 0), cig_input_state()->pointer.drag.change_last_frame);
      break;
    case 8: /* Button up */
      TEST_ASSERT_EQUAL(CIG_DRAG_STATE_INACTIVE, cig_dragged(CIG_INPUT_PRIMARY_ACTION));
      TEST_ASSERT_EQUAL(0, cig_input_state()->pointer.drag.id);
      TEST_ASSERT_FALSE(cig_input_state()->pointer.locked);
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
  int i;
  for (i = 0; i < 3; ++i) {
    begin(FRAME_TIME);

    switch (i) {
    case 0:
      cig_set_pointer_position(cig_v_make(25, 25));
      cig_set_pointer_state(0);
      break;
    case 1:
      cig_set_pointer_position(cig_v_make(30, 30));
      cig_set_pointer_state(CIG_INPUT_SECONDARY_ACTION);
      break;
    case 2:
      cig_set_pointer_position(cig_v_make(35, 35));
      cig_set_pointer_state(CIG_INPUT_SECONDARY_ACTION);
      break;
    }

    cig_enable_interaction();

    /* Drag state is not activated because the action type is not recognized */
    TEST_ASSERT_EQUAL(CIG_DRAG_STATE_INACTIVE, cig_dragged(CIG_INPUT_PRIMARY_ACTION));
    TEST_ASSERT_EQUAL(0, cig_input_state()->pointer.drag.id);

    end();
  }
}

TEST(core_input, button_states)
{
  begin(0);

  /* (Time 0) */
  cig_set_pointer_position(cig_v_zero());
  cig_set_pointer_state(CIG_INPUT_PRIMARY_ACTION);

  TEST_ASSERT_TRUE(cig_input_state()->pointer.action_mask & CIG_INPUT_PRIMARY_ACTION);
  TEST_ASSERT_EQUAL(CIG_INPUT_PRIMARY_ACTION, cig_input_state()->pointer.last_action_began);
  TEST_ASSERT_EQUAL(0, cig_input_state()->pointer.last_action_ended);
  TEST_ASSERT_EQUAL(BEGAN, cig_input_state()->pointer.click_state);

  /* (T1) */
  cig_set_pointer_position(cig_v_zero());
  cig_set_pointer_state(CIG_INPUT_PRIMARY_ACTION | CIG_INPUT_SECONDARY_ACTION);

  TEST_ASSERT_TRUE(cig_input_state()->pointer.action_mask & CIG_INPUT_ACTION_ANY);
  TEST_ASSERT_EQUAL(CIG_INPUT_SECONDARY_ACTION, cig_input_state()->pointer.last_action_began);
  TEST_ASSERT_EQUAL(0, cig_input_state()->pointer.last_action_ended);
  /* Maybe this should report a failed case or something because you're not
     pressing down both mouse buttons and expecting a click event normally? */
  TEST_ASSERT_EQUAL(NEITHER, cig_input_state()->pointer.click_state);

  /* (T2) */
  cig_set_pointer_position(cig_v_zero());
  cig_set_pointer_state(CIG_INPUT_SECONDARY_ACTION);
  
  TEST_ASSERT_TRUE(cig_input_state()->pointer.action_mask & CIG_INPUT_SECONDARY_ACTION);
  TEST_ASSERT_EQUAL(CIG_INPUT_PRIMARY_ACTION, cig_input_state()->pointer.last_action_ended);
  TEST_ASSERT_EQUAL(NEITHER, cig_input_state()->pointer.click_state);

  /* (T3) */
  cig_set_pointer_position(cig_v_zero());
  cig_set_pointer_state(0);
  
  TEST_ASSERT_EQUAL(0, cig_input_state()->pointer.action_mask);
  TEST_ASSERT_EQUAL(CIG_INPUT_SECONDARY_ACTION, cig_input_state()->pointer.last_action_ended);
  TEST_ASSERT_EQUAL(ENDED, cig_input_state()->pointer.click_state);

  end();
}

/**
 * Simulates backend only updating pointer state once when click begins and ends.
 * No pointer updates are sent on intermediate iterations.
 */
TEST(core_input, pointer_state_changes_only)
{
  int i;

  for (i = 0; i < 10; ++i) {
    begin(FRAME_TIME);

    cig_set_pointer_position(cig_v_make(25, 25));

    cig_enable_interaction();

    switch (i) {
    case 1: /* Click starts at T1 */
      cig_set_pointer_state(CIG_INPUT_PRIMARY_ACTION);
      break;
    case 9: /* Click ends at T8 */
      cig_set_pointer_state(0);
      TEST_ASSERT_TRUE(cig_clicked(CIG_INPUT_PRIMARY_ACTION, CIG_CLICK_DEFAULT_OPTIONS));
      break;
    default:
      break;
    }

    end();
  }
}

TEST(core_input, focus_not_enabled_safe)
{
  begin(0);
  TEST_ASSERT_FALSE(cig_focused());
  end();

  begin(0);
  cig_push_frame(RECT_AUTO);
  TEST_ASSERT_FALSE(cig_focused());
  cig_pop_frame();
  end();
}

/* Focus is activated internally by tracking which element gets clicked on */
TEST(core_input, focus_internal)
{
  int i;

  for (i = 0; i < 10; ++i) {
    begin(0);

    cig_set_pointer_position(cig_v_zero());

    if (i == 0) {
      cig_set_pointer_state(CIG_INPUT_PRIMARY_ACTION);
    } else {
      cig_set_pointer_state(0);
    }

    cig_enable_focus(NULL);

    switch (i) {
    /* Nothing can happen yet */
    case 0: TEST_ASSERT_FALSE(cig_focused()); break;

    /**
     * Current element was determined to be the last one to request
     * focus during the last iteration and receive the click.
     */
    case 1: TEST_ASSERT_TRUE(cig_focused()); break;

    /* From there on it will remain focused */
    default: TEST_ASSERT_TRUE(cig_focused()); break;
    }

    end();
  }
}

/* Focus can be stored and activated externally as well, driven by pointing to a boolean */
TEST(core_input, focus_external)
{
  bool is_focused = false;

  TEST_CYCLE(
    cig_enable_focus(&is_focused);
    TEST_ASSERT_FALSE(cig_focused());
  );

  is_focused = true;

  /* Like with internal click-to-focus, the element becomes focused after the next iteration */
  TEST_CYCLE(
    cig_enable_focus(&is_focused);
    TEST_ASSERT_FALSE(cig_focused());
  );

  TEST_CYCLE(
    cig_enable_focus(&is_focused);
    TEST_ASSERT_TRUE(cig_focused());
  );

  is_focused = false;

  /* However focus is *lost* immediately */
  TEST_CYCLE(
    cig_enable_focus(&is_focused);
    TEST_ASSERT_FALSE(cig_focused());
  );
}

/**
 * Focus activation / observation can also be mixed internally and externally.
 * 
 * Click activates it, updates the external state through the pointer
 * and can be deactivated through it as well.
 */
TEST(core_input, focus_internal_and_external)
{
  int i;
  bool is_focused = false;

  for (i = 0; i < 10; ++i) {
    begin(0);

    cig_set_pointer_position(cig_v_zero());

    if (i == 0) {
      cig_set_pointer_state(CIG_INPUT_PRIMARY_ACTION);
    } else {
      cig_set_pointer_state(0);
    }

    cig_enable_focus(&is_focused);

    switch (i) {
    case 0:
      TEST_ASSERT_FALSE(cig_focused());
      TEST_ASSERT_FALSE(is_focused);
      break;

    case 1:
      TEST_ASSERT_TRUE(cig_focused());
      TEST_ASSERT_TRUE(is_focused);
      break;

    case 2:
      is_focused = false;
      break;

    default:
      TEST_ASSERT_FALSE(cig_focused());
      TEST_ASSERT_FALSE(is_focused);
      break;
    }

    end();
  }
}

/* Parent focus affects child focus */
TEST(core_input, focus_chain)
{
  int i;

  for (i = 0; i < 10; ++i) {
    begin(0);

    switch (i) {
    case 0:
      /* Click at (25,25) */
      cig_set_pointer_position(cig_v_make(25, 25));
      cig_set_pointer_state(CIG_INPUT_PRIMARY_ACTION);
      break;
    case 5:
      /* Click at (75,25) */
      cig_set_pointer_position(cig_v_make(75, 25));
      cig_set_pointer_state(CIG_INPUT_PRIMARY_ACTION);
      break;
    default:
      cig_set_pointer_position(cig_v_zero());
      cig_set_pointer_state(0);
      break;
    }

    /* Parent */
    cig_push_frame(cig_r_make(0, 0, 100, 100));
    cig_enable_focus(NULL);

    if (i >= 1) {
      /* Parent gets focus when any child is focused */
      TEST_ASSERT_TRUE(cig_focused());
    }

    /* Child 1 */
    {
      cig_push_frame(cig_r_make(0, 0, 50, 50));
      cig_enable_focus(NULL);

      /* Child 1 is focused when clicking at (25,25) */
      if (i >= 1 && i < 6) {
        TEST_ASSERT_TRUE(cig_focused());
      } else {
        TEST_ASSERT_FALSE(cig_focused());
      }

      cig_pop_frame(); /* Child 1 */
    }

    /* Child 2 */
    {
      cig_push_frame(cig_r_make(50, 0, 50, 50));
      cig_enable_focus(NULL);

      /* Child 2 is focused when clicking at (75,25) */
      if (i >= 6) {
        TEST_ASSERT_TRUE(cig_focused());
      } else {
        TEST_ASSERT_FALSE(cig_focused());
      }

      cig_pop_frame(); /* Child 2 */
    }

    /* Child 3 doesn't have explicit focus state -- reads parent focus instead */
    {
      cig_push_frame(cig_r_make(0, 50, 100, 50));
      if (i >= 1) {
        TEST_ASSERT_TRUE(cig_focused());
      } else {
        TEST_ASSERT_FALSE(cig_focused());
      }
      cig_pop_frame(); /* Child 3 */
    }

    cig_pop_frame(); /* Parent */

    end();
  }
}

TEST(core_input, child_unfocuses_when_parent_unfocuses)
{
  int i;
  bool parent_focused = true, child_focused = true;

  for (i = 0; i < 3; ++i) {
    begin(0);

    cig_push_frame(RECT_AUTO); /* Parent */
    
    cig_enable_focus(&parent_focused);

    cig_push_frame(RECT_AUTO); /* Child */
    cig_enable_focus(&child_focused);
    cig_pop_frame(); /* Child */

    cig_pop_frame(); /* Parent */

    end();

    switch (i) {
    case 0:
      break;
    case 1:
      parent_focused = false;
      break;
    default:
      TEST_ASSERT_FALSE(child_focused);
      break;
    }
  }
}

TEST(core_input, gained_focus_event)
{
  int i;
  bool focused = true;

  for (i = 0; i < 10; ++i) {
    begin(0);

    cig_enable_focus(&focused);

    switch (i) {
    case 1:
      /* Only true for one iteration */
      TEST_ASSERT_TRUE(cig_gained_focus());
      break;
    default:
      TEST_ASSERT_FALSE(cig_gained_focus());
      break;
    }

    end();
  }
}

TEST(core_input, lost_focus_event)
{
  int i;
  bool focused = true;

  for (i = 0; i < 10; ++i) {
    begin(0);

    cig_enable_focus(&focused);

    switch (i) {
    case 1:
      focused = false;
      break;
    case 2:
      /* Only true for one iteration */
      TEST_ASSERT_TRUE(cig_lost_focus());
      break;
    default:
      TEST_ASSERT_FALSE(cig_lost_focus());
      break;
    }

    end();
  }
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
  RUN_TEST_CASE(core_input, pointer_state_changes_only);
  RUN_TEST_CASE(core_input, focus_not_enabled_safe);
  RUN_TEST_CASE(core_input, focus_internal);
  RUN_TEST_CASE(core_input, focus_external);
  RUN_TEST_CASE(core_input, focus_internal_and_external);
  RUN_TEST_CASE(core_input, focus_chain);
  RUN_TEST_CASE(core_input, child_unfocuses_when_parent_unfocuses);
  RUN_TEST_CASE(core_input, gained_focus_event);
  RUN_TEST_CASE(core_input, lost_focus_event);
}
