#include "unity.h"
#include "fixture.h"
#include "cigcore.h"
#include "asserts.h"
#include <string.h>

TEST_GROUP(core_state);

static cig_context ctx = { 0 };

TEST_SETUP(core_state) {
  cig_init_context(&ctx);
}

TEST_TEAR_DOWN(core_state) {}

static void begin() {
  cig_begin_layout(&ctx, NULL, cig_r_make(0, 0, 640, 480), 0.1f);
}

static void end() {
  cig_end_layout();
}

/*  ┌────────────┐
    │ TEST CASES │
    └────────────┘ */

TEST(core_state, status) {
  register int i;
  cig_id persistent_id = 0;

  for (i = 0; i < 2; ++i) {
    begin();
    cig_push_frame(RECT_AUTO);

    if (!persistent_id) { persistent_id = cig_current()->id; }
    else { TEST_ASSERT_EQUAL_UINT32(persistent_id, cig_current()->id); }

    cig_state *state = cig_enable_state();

    TEST_ASSERT_NOT_NULL(state);

    if (i == 0) { TEST_ASSERT_EQUAL(CIG_STATE_ACTIVATED, state->status); }
    else if (i == 1) { TEST_ASSERT_EQUAL(CIG_STATE_ACTIVE, state->status); }

    cig_pop_frame();
    end();
  }
}

TEST(core_state, pool_limit) {
  register int i;
  begin();
  
  for (i = 0; i < CIG_STATES_MAX + 1; ++i) {
    cig_push_frame(RECT_AUTO);
    
    if (i < CIG_STATES_MAX) {
      TEST_ASSERT_NOT_NULL(cig_enable_state());
    } else {
      TEST_ASSERT_NULL(cig_enable_state());
    }
    
    cig_pop_frame();
  }
  
  end();
}

TEST(core_state, stale) {
  register int i;
  begin();
  /*  Tick 1: Mark all states as used */
  for (i = 0; i < CIG_STATES_MAX + 1; ++i) {
    cig_push_frame(RECT_AUTO);
    CIG_UNUSED(cig_enable_state());
    cig_pop_frame();
  }
  end();

  /*  Tick 2: State is considered stale if it hasn't been used during the last tick,
      so at this point we don't expect there to be any available states */
  begin();
  cig_set_next_id(12345);
  cig_push_frame(RECT_AUTO);
  TEST_ASSERT_NULL(cig_enable_state());
  cig_pop_frame();
  end();

  /*  Tick 3: Now there should be */
  begin();
  cig_set_next_id(12345);
  cig_push_frame(RECT_AUTO);
  TEST_ASSERT_NOT_NULL(cig_enable_state());
  cig_pop_frame();
  end();
}

TEST(core_state, memory_arena) {
  register int i;
  for (i = 0; i < 2; ++i) {
    begin();

    TEST_ASSERT_EQUAL_UINT(0, cig_enable_state()->arena.mapped);

    cig_v *vec2 = (cig_v *)cig_arena_allocate(NULL, sizeof(cig_v));
    unsigned long *ul = (unsigned long *)cig_arena_allocate(NULL, sizeof(unsigned long));
    char *str = (char *)cig_arena_allocate(NULL, sizeof(char[32]));
    void *hundred_whole_kilobytes = cig_arena_allocate(NULL, sizeof(char[1024*100]));

    TEST_ASSERT_NULL(hundred_whole_kilobytes); /* Doesn't fit */

    TEST_ASSERT_EQUAL_UINT(
      sizeof(cig_v) + sizeof(unsigned long) + sizeof(char[32]),
      cig_enable_state()->arena.mapped
    );

    if (i == 0) { /* Store data */
      *vec2 = cig_v_make(13, 17);
      *ul = cig_current()->id;
      strcpy(str, "Hello, World!");
    } else { /* Read data */
      TEST_ASSERT_EQUAL_VEC2(cig_v_make(13, 17), *vec2);
      TEST_ASSERT_EQUAL_UINT32(cig_current()->id, *ul);
      TEST_ASSERT_EQUAL_STRING("Hello, World!", str);
    }

    end();
  }
}

TEST(core_state, memory_arena_read) {
  register int i;
  begin();

  /*  Write some values */
  {
    cig_v *vec2 = (cig_v *)cig_arena_allocate(NULL, sizeof(cig_v));
    unsigned long *ul = (unsigned long *)cig_arena_allocate(NULL, sizeof(unsigned long));
    char *str = (char *)cig_arena_allocate(NULL, sizeof(char[32]));

    *vec2 = cig_v_make(13, 17);
    *ul = cig_current()->id;
    strcpy(str, "Hello, World!");
  }

  /*  Read them a couple of times */
  for (i = 0; i < 2; ++i) {
    cig_v *vec2 = (cig_v *)cig_arena_read(NULL, true, sizeof(cig_v));
    unsigned long *ul = (unsigned long *)cig_arena_read(NULL, false, sizeof(unsigned long));
    char *str = (char *)cig_arena_read(NULL, false, sizeof(char[32]));

    TEST_ASSERT_EQUAL_VEC2(cig_v_make(13, 17), *vec2);
    TEST_ASSERT_EQUAL_UINT32(cig_current()->id, *ul);
    TEST_ASSERT_EQUAL_STRING("Hello, World!", str);
  }

  end();
}

TEST_GROUP_RUNNER(core_state) {
  RUN_TEST_CASE(core_state, status);
  RUN_TEST_CASE(core_state, pool_limit);
  RUN_TEST_CASE(core_state, stale);
  RUN_TEST_CASE(core_state, memory_arena);
  RUN_TEST_CASE(core_state, memory_arena_read);
}