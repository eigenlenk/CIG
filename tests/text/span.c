#include "unity.h"
#include "fixture.h"
#include "cigtext.h"
// #include "asserts.h"

TEST_GROUP(text_span);

static cig_context_t ctx = { 0 };
static void text_renderer(cig_rect_t, const char*, size_t);

TEST_SETUP(text_span) {
  cig_init_context(&ctx);
}

TEST_TEAR_DOWN(text_span) {}

static void begin() {
	cig_begin_layout(&ctx, NULL, cig_rect_make(0, 0, 640, 480));
}

static void end() {
	cig_end_layout();
}

TEST(text_span, basic_span) {
  int spans = 0;
  
  void text_render(cig_rect_t rect, const char *str, size_t len) {
    spans ++;
  }
  
  cig_vec2_t text_measure(const char *str, size_t len) {
    return cig_vec2_make(len, 1); /* W, H in characters for terminal display */
  }
  
  cig_set_text_render_callback(&text_render);
  cig_set_text_measure_callback(&text_measure);

  for (int i = 0; i < 2; ++i, spans = 0) {
    begin();
    cig_label("Olá mundo!");
    
    if (cig_push_frame(CIG_CENTERED(10, 10))) {
      cig_label("Olá mundo!");
      cig_pop_frame();
    }

    // TEST_ASSERT_EQUAL(2, spans);

    end();
  }
}

TEST_GROUP_RUNNER(text_span) {
  RUN_TEST_CASE(text_span, basic_span);
}