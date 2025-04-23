#include "unity.h"
#include "fixture.h"
#include "cigtext.h"
#include "cigcorem.h"
#include "asserts.h"
#include "utf8.h"

TEST_GROUP(text_label);

static cig_context_t ctx;
static int text_measure_calls;
static struct {
  cig_rect_t rects[128];
  size_t count;
} spans;

CIG_INLINED void text_render(cig_rect_t rect, const char *str, size_t len, cig_font_ref font, cig_text_color_ref color) {
  spans.rects[spans.count++] = rect;
}

CIG_INLINED cig_vec2_t text_measure(const char *str, size_t len, cig_font_ref font) {
  utf8_string slice = (utf8_string) { str, len };
  text_measure_calls ++;
  // printf("%.*s = %d\n", len, str, utf8_char_count(slice));
  return cig_vec2_make(utf8_char_count(slice), 1);
}

CIG_INLINED cig_font_info_t font_query(cig_font_ref font_ref) {
  return (cig_font_info_t) {
    .height = 1,
    .line_spacing = 0,
    .baseline_offset = 0,
    .word_spacing = 1
  };
}

TEST_SETUP(text_label) {
  cig_init_context(&ctx);
  
  cig_set_text_render_callback(&text_render);
  cig_set_text_measure_callback(&text_measure);
  cig_set_font_query_callback(&font_query);
  
  spans.count = 0;
  text_measure_calls = 0;
}

TEST_TEAR_DOWN(text_label) {}

static void begin() {
  /* In the context of these tests we work with a terminal/text-mode where
     bounds and positions are calculated in number of characters rather than pixels */
	cig_begin_layout(&ctx, NULL, cig_rect_make(0, 0, 80, 25)); /* 80 x 25 character terminal */
}

static void end() {
	cig_end_layout();
}

TEST(text_label, basic_label) {  
  /* Runing to iterations to test that text is measured only once
     and cached data is used on consecutive layout passes */
  for (int i = 0; i < 2; ++i) {
    begin();
    
    /* Span is an atomic text component, a single word basically. It can override font & color
       provided by text properties by default */
    spans.count = 0;
    
    /* Label centers text both horizontally and vertically by default */
    cig_label((cig_text_properties_t) { }, "Olá mundo!");
    
    TEST_ASSERT_EQUAL(2, spans.count);
    TEST_ASSERT_EQUAL_RECT(cig_rect_make(35, 12, 3, 1), spans.rects[0]);
    TEST_ASSERT_EQUAL_RECT(cig_rect_make(39, 12, 6, 1), spans.rects[1]);
    
    if (cig_push_frame(CIG_CENTERED(40, 15))) {
      spans.count = 0;
      cig_label((cig_text_properties_t) {
        .alignment.horizontal = CIG_TEXT_ALIGN_LEFT
      }, "Olá mundo!");
      TEST_ASSERT_EQUAL_RECT(cig_rect_make(20, 12, 3, 1), spans.rects[0]);
      TEST_ASSERT_EQUAL_RECT(cig_rect_make(24, 12, 6, 1), spans.rects[1]);
      TEST_ASSERT_EQUAL(2, spans.count);
      cig_pop_frame();
    }

    end();
  }
  
  TEST_ASSERT_EQUAL(4, text_measure_calls); /* 2 labels with 2 spans each */
}

TEST(text_label, horizontal_alignment_left) {
  begin();
  
  CIG({
    CIG_RECT(CIG_FILL_H(1))
  }) {
    cig_label((cig_text_properties_t) {
      .alignment.horizontal = CIG_TEXT_ALIGN_LEFT
    }, "Label");
    
    TEST_ASSERT_EQUAL_RECT(cig_rect_make(0, 0, 5, 1), spans.rects[0]);
  }
}

TEST(text_label, horizontal_alignment_center) {
  begin();
  
  CIG({
    CIG_RECT(CIG_FILL_H(1))
  }) {
    cig_label((cig_text_properties_t) {
      .alignment.horizontal = CIG_TEXT_ALIGN_CENTER
    }, "Label");
    
    TEST_ASSERT_EQUAL_RECT(cig_rect_make(37, 0, 5, 1), spans.rects[0]);
  }
}

TEST(text_label, horizontal_alignment_right) {
  begin();
  
  CIG({
    CIG_RECT(CIG_FILL_H(1))
  }) {
    cig_label((cig_text_properties_t) {
      .alignment.horizontal = CIG_TEXT_ALIGN_RIGHT
    }, "Label");
    
    TEST_ASSERT_EQUAL_RECT(cig_rect_make(75, 0, 5, 1), spans.rects[0]);
  }
}

TEST(text_label, vertical_alignment_top) {
  begin();
  
  CIG({
    CIG_RECT(CIG_FILL_W(5))
  }) {
    cig_label((cig_text_properties_t) {
      .alignment.vertical = CIG_TEXT_ALIGN_TOP
    }, "Label");
    
    TEST_ASSERT_EQUAL_RECT(cig_rect_make(0, 0, 5, 1), spans.rects[0]);
  }
}

TEST(text_label, vertical_alignment_middle) {
  begin();
  
  CIG({
    CIG_RECT(CIG_FILL_W(5))
  }) {
    cig_label((cig_text_properties_t) {
      .alignment.vertical = CIG_TEXT_ALIGN_MIDDLE
    }, "Label");
    
    TEST_ASSERT_EQUAL_RECT(cig_rect_make(0, 12, 5, 1), spans.rects[0]);
  }
}

TEST(text_label, vertical_alignment_bottom) {
  begin();
  
  CIG({
    CIG_RECT(CIG_FILL_W(5))
  }) {
    cig_label((cig_text_properties_t) {
      .alignment.vertical = CIG_TEXT_ALIGN_BOTTOM
    }, "Label");
    
    TEST_ASSERT_EQUAL_RECT(cig_rect_make(0, 24, 5, 1), spans.rects[0]);
  }
}

TEST(text_label, forced_line_change) {
  begin();
  
  CIG({
    CIG_RECT(cig_rect_make(0, 0, 8, 2))
  }) {
    /* Left aligned text */
    CIG(_) {
      cig_label((cig_text_properties_t) {
        .alignment.horizontal = CIG_TEXT_ALIGN_LEFT,
        .alignment.vertical = CIG_TEXT_ALIGN_TOP
      }, "Olá mundo!");
      
      /* ╔════════╗  
         ║Olá_____║  
         ║mundo!__║  
         ╚════════╝ */
      TEST_ASSERT_EQUAL_RECT(cig_rect_make(0, 0, 3, 1), spans.rects[0]);
      TEST_ASSERT_EQUAL_RECT(cig_rect_make(0, 1, 6, 1), spans.rects[1]);
    }
    
    /* Centered text */
    CIG(_) {
      cig_label((cig_text_properties_t) {
        .alignment.vertical = CIG_TEXT_ALIGN_TOP
      }, "Hello world!");
      
      /* ╔════════╗  
         ║_Hello__║  
         ║_world!_║  
         ╚════════╝ */
      TEST_ASSERT_EQUAL_RECT(cig_rect_make(1, 0, 5, 1), spans.rects[2]);
      TEST_ASSERT_EQUAL_RECT(cig_rect_make(1, 1, 6, 1), spans.rects[3]);
    }
    
    /* Right aligned text */
    CIG(_) {
      cig_label((cig_text_properties_t) {
        .alignment.horizontal = CIG_TEXT_ALIGN_RIGHT,
        .alignment.vertical = CIG_TEXT_ALIGN_TOP
      }, "Tere maailm!");
      
      /* ╔════════╗  
         ║____Tere║  
         ║_maailm!║  
         ╚════════╝ */
      TEST_ASSERT_EQUAL_RECT(cig_rect_make(4, 0, 4, 1), spans.rects[4]);
      TEST_ASSERT_EQUAL_RECT(cig_rect_make(1, 1, 7, 1), spans.rects[5]);
    }
  }
}

TEST_GROUP_RUNNER(text_label) {
  RUN_TEST_CASE(text_label, basic_label);
  RUN_TEST_CASE(text_label, horizontal_alignment_left);
  RUN_TEST_CASE(text_label, horizontal_alignment_center);
  RUN_TEST_CASE(text_label, horizontal_alignment_right);
  RUN_TEST_CASE(text_label, vertical_alignment_top);
  RUN_TEST_CASE(text_label, vertical_alignment_middle);
  RUN_TEST_CASE(text_label, vertical_alignment_bottom);
  RUN_TEST_CASE(text_label, forced_line_change);
}