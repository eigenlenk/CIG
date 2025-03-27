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

static int spans = 0;

CIG_INLINED void text_render(cig_rect_t rect, const char *str, size_t len) {
  spans ++;
}

CIG_INLINED cig_vec2_t text_measure(const char *str, size_t len) {
  return cig_vec2_make(10*len, 10); /* 10x10 glyphs */
}

CIG_INLINED cig_font_info_t font_query(cig_font_ref font_ref) {
  return (cig_font_info_t) { 10, 0 };
}

TEST(text_span, basic_label) {  
  cig_set_text_render_callback(&text_render);
  cig_set_text_measure_callback(&text_measure);
  cig_set_font_query_callback(&font_query);

  for (int i = 0; i < 2; ++i, spans = 0) {
    begin();
    
    spans = 0;
    cig_label("Olá mundo!", CIG_TEXT_ALIGN_CENTER);
    
    TEST_ASSERT_EQUAL(2, spans);
    
    if (cig_push_frame(CIG_CENTERED(50, 50))) {
      spans = 0;
      cig_label("This is a text.", CIG_TEXT_ALIGN_CENTER);
      TEST_ASSERT_EQUAL(4, spans);
      cig_pop_frame();
    }

    end();
  }
}

TEST_GROUP_RUNNER(text_span) {
  RUN_TEST_CASE(text_span, basic_label);
}