#include "unity.h"
#include "fixture.h"
#include "cigtext.h"
#include "cigcorem.h"
#include "asserts.h"
#include "utf8.h"

TEST_GROUP(text_label);

static cig_context ctx;
static int text_measure_calls;
static struct {
  cig_r rects[16];
  char strings[16][128];
  size_t count;
} spans;

CIG_INLINED void text_render(
  const char *str,
  size_t len,
  cig_r rect,
  cig_font_ref font,
  cig_text_color_ref color,
  cig_text_style style
) {
  int i = spans.count++;
  spans.rects[i] = rect;
  sprintf(spans.strings[i], "%.*s", (uint32_t)len, str);
}

CIG_INLINED cig_v text_measure(
  const char *str,
  size_t len,
  cig_font_ref font,
  cig_text_style style
) {
  utf8_string slice = (utf8_string) { str, len };
  text_measure_calls ++;
  // printf("MEASURE: %.*s = %d\n", len, str, utf8_char_count(slice));
  return cig_v_make(utf8_char_count(slice), 1);
}

CIG_INLINED cig_font_info font_query(cig_font_ref font_ref) {
  return (cig_font_info) {
    .height = 1,
    .baseline_offset = 0
  };
}

TEST_SETUP(text_label) {
  cig_init_context(&ctx);

  cig_assign_draw_text(&text_render);
  cig_assign_measure_text(&text_measure);
  cig_assign_query_font(&font_query);

  text_measure_calls = 0;
}

TEST_TEAR_DOWN(text_label) {}

static void begin() {
  /*  In the context of these tests we work with a terminal/text-mode where
      bounds and positions are calculated in number of characters rather than pixels */
	cig_begin_layout(&ctx, NULL, cig_r_make(0, 0, 80, 25), 0.1f); /* 80 x 25 character terminal */

  spans.count = 0;
}

static void end() {
	cig_end_layout();
}

/*  ┌────────────┐
    │ TEST CASES │
    └────────────┘ */

TEST(text_label, fits_arena) {
  begin();
  TEST_ASSERT_NOT_NULL(CIG_ALLOCATE(cig_label));
}

TEST(text_label, single) {  
  register int i;
  /*  Runing to iterations to test that text is measured only once
      and cached data is used on consecutive layout passes */
  for (i = 0; i < 2; ++i) {
    begin();

    /*  Label centers text both horizontally and vertically by default.
        This label will consist of a single span */
    cig_draw_label((cig_text_properties) { 0 }, "Olá mundo!");

    /*  Span is an atomic text component, a piece of text that runs until
        the horizontal bounds of the label, or until some property of the
        text changes (font, color, link etc.) */
    TEST_ASSERT_EQUAL(1, spans.count);
    TEST_ASSERT_EQUAL_RECT(cig_r_make(35, 12, 10, 1), spans.rects[0]);

    end();
  }

  /*  Measure is called after every space, among others, to keep a reference
      until the text no longer fits */
  TEST_ASSERT_EQUAL(2, text_measure_calls);
}

TEST(text_label, multiline) {
  register int i;
  for (i = 0; i < 2; ++i) {
    begin();

    cig_draw_label((cig_text_properties) { 0 }, "Olá mundo!\nHello world!");

    TEST_ASSERT_EQUAL(2, spans.count);
    TEST_ASSERT_EQUAL_RECT(cig_r_make(35, 11, 10, 1), spans.rects[0]);
    TEST_ASSERT_EQUAL_RECT(cig_r_make(34, 12, 12, 1), spans.rects[1]);

    end();
  }

  TEST_ASSERT_EQUAL(4, text_measure_calls);
}

TEST(text_label, span_limit)
{
  begin();

  /* We allocate 2 spans/lines. Third line in the text is not added to the label */
  cig_label *label = cig_arena_allocate(NULL, sizeof(cig_label));
  label->spans = CIG_ALLOCATE(cig_span[2]); /* Max 2 lines */
  label->available_spans = 2;

  cig_label_prepare(label, cig_v_make(15, 3), (cig_text_properties) { 0 }, "Olá mundo!\nHello world!\nTere maailm!");

  TEST_ASSERT_EQUAL(2, label->span_count);
  TEST_ASSERT_EQUAL_STRING("Olá mundo!", spans.strings[0]);
  TEST_ASSERT_EQUAL_STRING("Hello world!", spans.strings[1]);

  end();
}

TEST(text_label, horizontal_alignment_left) {
  begin();

  CIG(RECT_AUTO_H(1)) {
    cig_draw_label((cig_text_properties) {
      .alignment.horizontal = CIG_TEXT_ALIGN_LEFT
    }, "Label");

    TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 0, 5, 1), spans.rects[0]);
  }
}

TEST(text_label, horizontal_alignment_center) {
  begin();

  CIG(RECT_AUTO_H(1)) {
    cig_draw_label((cig_text_properties) {
      .alignment.horizontal = CIG_TEXT_ALIGN_CENTER
    }, "Label");

    TEST_ASSERT_EQUAL_RECT(cig_r_make(37, 0, 5, 1), spans.rects[0]);
  }
}

TEST(text_label, horizontal_alignment_right) {
  begin();

  CIG(RECT_AUTO_H(1)) {
    cig_draw_label((cig_text_properties) {
      .alignment.horizontal = CIG_TEXT_ALIGN_RIGHT
    }, "Label");

    TEST_ASSERT_EQUAL_RECT(cig_r_make(75, 0, 5, 1), spans.rects[0]);
  }
}

TEST(text_label, vertical_alignment_top) {
  begin();

  CIG(RECT_AUTO_W(5)) {
    cig_draw_label((cig_text_properties) {
      .alignment.vertical = CIG_TEXT_ALIGN_TOP
    }, "Label");

    TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 0, 5, 1), spans.rects[0]);
  }
}

TEST(text_label, vertical_alignment_middle) {
  begin();

  CIG(RECT_AUTO_W(5)) {
    cig_draw_label((cig_text_properties) {
      .alignment.vertical = CIG_TEXT_ALIGN_MIDDLE
    }, "Label");

    TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 12, 5, 1), spans.rects[0]);
  }
}

TEST(text_label, vertical_alignment_bottom) {
  begin();

  CIG(RECT_AUTO_W(5)) {
    cig_draw_label((cig_text_properties) {
      .alignment.vertical = CIG_TEXT_ALIGN_BOTTOM
    }, "Label");

    TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 24, 5, 1), spans.rects[0]);
  }
}

TEST(text_label, forced_line_change) {
  begin();

  CIG(cig_r_make(0, 0, 8, 2)) {
    /*  Left aligned text */
    CIG(_) {
      cig_draw_label((cig_text_properties) {
        .alignment.horizontal = CIG_TEXT_ALIGN_LEFT,
        .alignment.vertical = CIG_TEXT_ALIGN_TOP
      }, "Olá mundo!");

      /*  ╔════════╗  
          ║Olá_____║  
          ║mundo!__║  
          ╚════════╝ */
      TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 0, 3, 1), spans.rects[0]);
      TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 1, 6, 1), spans.rects[1]);
    }

    /*  Centered text */
    CIG(_) {
      cig_draw_label((cig_text_properties) {
        .alignment.vertical = CIG_TEXT_ALIGN_TOP
      }, "Hello world!");

      /*  ╔════════╗  
          ║_Hello__║  
          ║_world!_║  
          ╚════════╝ */
      TEST_ASSERT_EQUAL_RECT(cig_r_make(1, 0, 5, 1), spans.rects[2]);
      TEST_ASSERT_EQUAL_RECT(cig_r_make(1, 1, 6, 1), spans.rects[3]);
    }
    
    /*  Right aligned text */
    CIG(_) {
      cig_draw_label((cig_text_properties) {
        .alignment.horizontal = CIG_TEXT_ALIGN_RIGHT,
        .alignment.vertical = CIG_TEXT_ALIGN_TOP
      }, "Tere maailm!");

      /*  ╔════════╗  
          ║____Tere║  
          ║_maailm!║  
          ╚════════╝ */
      TEST_ASSERT_EQUAL_RECT(cig_r_make(4, 0, 4, 1), spans.rects[4]);
      TEST_ASSERT_EQUAL_RECT(cig_r_make(1, 1, 7, 1), spans.rects[5]);
    }
  }
}

TEST(text_label, prepare_single_long_word) {
  begin();

  cig_span span_list[4];
  cig_label label = { .spans = span_list, .available_spans = 4 };
  cig_label_prepare(&label, cig_v_make(7, 1), (cig_text_properties) { 0 }, "Foobarbaz");

  TEST_ASSERT_EQUAL_INT(9, label.bounds.w);
  TEST_ASSERT_EQUAL_INT(1, label.bounds.h);

  CIG(cig_r_make(0, 0, 7, 1)) {
    cig_label_draw(&label);

    TEST_ASSERT_EQUAL_RECT(cig_r_make(-1, 0, 9, 1), spans.rects[0]);
  }
}

TEST(text_label, prepare_multiple_long_words) {
  begin();

  cig_span span_list[4];
  cig_label label = { .spans = span_list, .available_spans = 4 };
  cig_label_prepare(&label, cig_v_make(7, 1), (cig_text_properties) { 0 }, "Foobarbaz barbazfoo bazfoobar");

  TEST_ASSERT_EQUAL_INT(9, label.bounds.w);
  TEST_ASSERT_EQUAL_INT(3, label.bounds.h);

  CIG(cig_r_make(0, 0, 7, 3)) {
    cig_label_draw(&label);

    TEST_ASSERT_EQUAL_RECT(cig_r_make(-1, 0, 9, 1), spans.rects[0]);
    TEST_ASSERT_EQUAL_RECT(cig_r_make(-1, 1, 9, 1), spans.rects[1]);
    TEST_ASSERT_EQUAL_RECT(cig_r_make(-1, 2, 9, 1), spans.rects[2]);
  }
}

TEST(text_label, overflow_enabled) {
  begin();
  CIG(RECT(0, 0, 16, 1)) {
    /*  Text overflow is enabled by default */
    cig_draw_label((cig_text_properties) {
      .alignment.horizontal = CIG_TEXT_ALIGN_LEFT,
      .max_lines = 1,
    }, "This text is going places");

    TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 0, 25, 1), spans.rects[0]);
    TEST_ASSERT_EQUAL_STRING("This text is going places", spans.strings[0]);
  }
  end();
}

TEST(text_label, single_line_overflow_truncate) {
  begin();
  CIG(RECT(0, 0, 16, 1)) {
    cig_draw_label((cig_text_properties) {
      .alignment.horizontal = CIG_TEXT_ALIGN_LEFT,
      .max_lines = 1,
      .overflow = CIG_TEXT_TRUNCATE
    }, "Text becomes truncated");

    TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 0, 16, 1), spans.rects[0]);
    TEST_ASSERT_EQUAL_STRING("Text becomes tru", spans.strings[0]);
  }
  end();
}

TEST(text_label, single_line_overflow_ellipsis) {
  begin();
  CIG(RECT(0, 0, 16, 1)) {
    cig_draw_label((cig_text_properties) {
      .alignment.horizontal = CIG_TEXT_ALIGN_LEFT,
      .max_lines = 1,
      .overflow = CIG_TEXT_SHOW_ELLIPSIS
    }, "Lorem ipsum dolor sit");

    TEST_ASSERT_EQUAL_RECT(cig_r_make(0, 0, 13, 1), spans.rects[0]);
    TEST_ASSERT_EQUAL_STRING("Lorem ipsum d", spans.strings[0]);
    TEST_ASSERT_EQUAL_STRING("...", spans.strings[1]);
  }
  end();
}

TEST(text_label, multiline_overflow_truncate) {
  begin();
  CIG(RECT(0, 0, 18, 1)) {
    cig_draw_label((cig_text_properties) {
      .alignment.horizontal = CIG_TEXT_ALIGN_LEFT,
      .max_lines = 2,
      .overflow = CIG_TEXT_TRUNCATE
    }, "Lorem ipsum dolor sit amet, consectetur.");

    TEST_ASSERT_EQUAL_STRING("Lorem ipsum dolor", spans.strings[0]);
    TEST_ASSERT_EQUAL_STRING("sit amet, consecte", spans.strings[1]);
  }
  end();
}

TEST(text_label, multiline_overflow_ellipsis) {
  begin();
  CIG(RECT(0, 0, 18, 1)) {
    cig_draw_label((cig_text_properties) {
      .alignment.horizontal = CIG_TEXT_ALIGN_LEFT,
      .max_lines = 2,
      .overflow = CIG_TEXT_SHOW_ELLIPSIS
    }, "Lorem ipsum dolor sit amet, consectetur.");

    TEST_ASSERT_EQUAL_STRING("Lorem ipsum dolor", spans.strings[0]);
    TEST_ASSERT_EQUAL_STRING("sit amet, conse", spans.strings[1]);
    TEST_ASSERT_EQUAL_STRING("...", spans.strings[2]);
  }
  end();
}

/* Raw text is for measuring and drawing smaller strings directly without caching anything internally */
TEST(text_label, raw_text) {
  begin();

  cig_v bounds = cig_measure_raw_text(NULL, 0, "Olá mundo!");
  TEST_ASSERT_EQUAL_VEC2(cig_v_make(10, 1), bounds);

  /* We have already calculated the text size, so we can just pass it in */
  cig_draw_raw_text(cig_v_make(5, 5), bounds, NULL, 0, NULL, "Olá mundo!");
  TEST_ASSERT_EQUAL_RECT(cig_r_make(5, 5, 10, 1), spans.rects[0]);

  /* But size can also be calculated automatically */
  cig_draw_raw_text(cig_v_make(6, 6), CIG_RAW_TEXT_AUTOMATIC_SIZE, NULL, 0, NULL, "Olá mundo!");
  TEST_ASSERT_EQUAL_RECT(cig_r_make(6, 6, 10, 1), spans.rects[1]);
}

TEST(text_label, raw_text_formatted) {
  begin();

  cig_v bounds = cig_measure_raw_text_formatted(NULL, 0, "Price of eggs: %.2f$", 130.45000);
  TEST_ASSERT_EQUAL_VEC2(cig_v_make(22, 1), bounds);

  cig_draw_raw_text_formatted(cig_v_make(5, 5), bounds, NULL, 0, NULL, "Price of eggs: %.2f$", 130.45000);
  TEST_ASSERT_EQUAL_STRING("Price of eggs: 130.45$", spans.strings[0]);
  TEST_ASSERT_EQUAL_RECT(cig_r_make(5, 5, 22, 1), spans.rects[0]);
}

TEST_GROUP_RUNNER(text_label) {
  RUN_TEST_CASE(text_label, fits_arena);
  RUN_TEST_CASE(text_label, single);
  RUN_TEST_CASE(text_label, multiline);
  RUN_TEST_CASE(text_label, span_limit);
  RUN_TEST_CASE(text_label, horizontal_alignment_left);
  RUN_TEST_CASE(text_label, horizontal_alignment_center);
  RUN_TEST_CASE(text_label, horizontal_alignment_right);
  RUN_TEST_CASE(text_label, vertical_alignment_top);
  RUN_TEST_CASE(text_label, vertical_alignment_middle);
  RUN_TEST_CASE(text_label, vertical_alignment_bottom);
  RUN_TEST_CASE(text_label, forced_line_change);
  RUN_TEST_CASE(text_label, prepare_single_long_word);
  RUN_TEST_CASE(text_label, prepare_multiple_long_words);
  RUN_TEST_CASE(text_label, overflow_enabled);
  RUN_TEST_CASE(text_label, single_line_overflow_truncate);
  RUN_TEST_CASE(text_label, single_line_overflow_ellipsis);
  RUN_TEST_CASE(text_label, multiline_overflow_truncate);
  RUN_TEST_CASE(text_label, multiline_overflow_ellipsis);
  RUN_TEST_CASE(text_label, raw_text);
  RUN_TEST_CASE(text_label, raw_text_formatted);
}
