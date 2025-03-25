#include "unity.h"
#include "fixture.h"
#include "cigcore.h"
#include "asserts.h"

TEST_GROUP(core_state);

static cig_context_t ctx = { 0 };

TEST_SETUP(core_state) {
  cig_init_context(&ctx);
}

TEST_TEAR_DOWN(core_state) {}

static void begin() {
	cig_begin_layout(&ctx, NULL, cig_rect_make(0, 0, 640, 480));
}

static void end() {
	cig_end_layout();
}

TEST(core_state, activation_states) {
  cig_id_t persistent_id = 0;
  
  for (int i = 0; i < 2; ++i) {
		begin();
    
    cig_push_frame(CIG_FILL);
    
    if (!persistent_id) { persistent_id = cig_frame()->id; }
    else { TEST_ASSERT_EQUAL_UINT32(persistent_id, cig_frame()->id); }
    
    cig_state_t *state = cig_state();
    
    TEST_ASSERT_NOT_NULL(state);
    
    if (i == 0) { TEST_ASSERT_EQUAL(ACTIVATED, state->activation_state); }
    else if (i == 1) { TEST_ASSERT_EQUAL(ACTIVE, state->activation_state); }
    
    cig_pop_frame();
    
    end();
  }
}

TEST(core_state, pool_limit) {
  begin();
  
  for (int i = 0; i < CIG_STATES_MAX + 1; ++i) {
    cig_push_frame(CIG_FILL);
    
    if (i < CIG_STATES_MAX) {
      TEST_ASSERT_NOT_NULL(cig_state());
    } else {
      TEST_ASSERT_NULL(cig_state());
    }
    
    cig_pop_frame();
  }
  
  end();
}

TEST(core_state, stale) {
  begin();
  /* Tick 1: Mark all states as used */
  for (int i = 0; i < CIG_STATES_MAX + 1; ++i) {
    cig_push_frame(CIG_FILL);
    CIG_UNUSED(cig_state());
    cig_pop_frame();
  }
  end();
  
  /* Tick 2: State is considered stale if it hasn't been used during the last tick,
     so at this point we don't expect there to be any available states */
  begin();
  cig_set_next_id(12345);
  cig_push_frame(CIG_FILL);
  TEST_ASSERT_NULL(cig_state());
  cig_pop_frame();
  end();
  
  /* Tick 3: Now there should be */
  begin();
  cig_set_next_id(12345);
  cig_push_frame(CIG_FILL);
  TEST_ASSERT_NOT_NULL(cig_state());
  cig_pop_frame();
  end();
}

TEST_GROUP_RUNNER(core_state) {
  RUN_TEST_CASE(core_state, activation_states);
  RUN_TEST_CASE(core_state, pool_limit);
  RUN_TEST_CASE(core_state, stale);
}