#include "unity.h"
#include "fixture.h"
#include "cigtext.h"
#include "cigcorem.h"
#include "asserts.h"
#include "utf8.h"
#include <string.h>

TEST_GROUP(text_style);

static cig_context_t ctx;
static int fonts[2];
static int red_color;

static struct {
  struct info {
    cig_rect_t rect;
    cig_font_ref font;
    cig_text_color_ref color;
    cig_text_style_t style;
    char str[256];
  } info[32];
  size_t count;
} spans;

CIG_INLINED void text_render(
  const char *str,
  size_t len,
  cig_rect_t rect,
  cig_font_ref font,
  cig_text_color_ref color,
  cig_text_style_t style
) {
  spans.info[spans.count++] = (struct info) {
    .rect = rect,
    .font = font,
    .color = color,
    .style = style
  };
  strncat(spans.info[spans.count-1].str, str, len);
}

CIG_INLINED cig_vec2_t text_measure(
  const char *str,
  size_t len,
  cig_font_ref font,
  cig_text_style_t style
) {
  utf8_string slice = (utf8_string) { str, len };
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

TEST_SETUP(text_style) {
  cig_init_context(&ctx);
  
  cig_set_text_render_callback(&text_render);
  cig_set_text_measure_callback(&text_measure);
  cig_set_font_query_callback(&font_query);
  cig_set_default_font(&fonts[0]);
  
  spans.count = 0;
}

TEST_TEAR_DOWN(text_style) {}

static void begin() {
  /* In the context of these tests we work with a terminal/text-mode where
     bounds and positions are calculated in number of characters rather than pixels */
	cig_begin_layout(&ctx, NULL, cig_rect_make(0, 0, 80, 25)); /* 80 x 25 character terminal */
}

TEST(text_style, font_override) {  
  begin();
  
  cig_label((cig_text_properties_t) {
    .flags = CIG_TEXT_FORMATTED
  }, "Overriding font for <font=%x>IMPACT</font>", &fonts[1]);
  
  TEST_ASSERT_EQUAL_PTR(&fonts[0], spans.info[0].font); /* Default font */
  TEST_ASSERT_EQUAL_PTR(&fonts[1], spans.info[3].font); /* Override */
}

TEST(text_style, color_override) {  
  begin();
  
  cig_label((cig_text_properties_t) {
    .flags = CIG_TEXT_FORMATTED
  }, "Roll out the <color=%x>red</color> carpet", &red_color);
  
  TEST_ASSERT_NULL(spans.info[0].color);
  TEST_ASSERT_EQUAL_PTR(&red_color, spans.info[3].color); /* Override */
}

TEST(text_style, bold) {  
  begin();
  
  cig_label((cig_text_properties_t) {
    .flags = CIG_TEXT_FORMATTED
  }, "I <b>know</b>");
  
  TEST_ASSERT_BITS_HIGH(CIG_TEXT_STYLE_BOLD, spans.info[1].style);
}

TEST(text_style, italic) {  
  begin();
  
  cig_label((cig_text_properties_t) {
    .flags = CIG_TEXT_FORMATTED
  }, "I <i>think</i>");
  
  TEST_ASSERT_BITS_HIGH(CIG_TEXT_STYLE_ITALIC, spans.info[1].style);
}

TEST(text_style, underline) {  
  begin();
  
  cig_label((cig_text_properties_t) {
    .flags = CIG_TEXT_FORMATTED
  }, "Click <u>here</u>");
  
  TEST_ASSERT_BITS_HIGH(CIG_TEXT_STYLE_UNDERLINE, spans.info[1].style);
}

TEST(text_style, strikethrough) {  
  begin();
  
  cig_label((cig_text_properties_t) {
    .flags = CIG_TEXT_FORMATTED
  }, "<s>Fixed</s>");
  
  TEST_ASSERT_BITS_HIGH(CIG_TEXT_STYLE_STRIKETHROUGH, spans.info[0].style);
}

TEST(text_style, unclosed_tag) {  
  begin();
  
  cig_label((cig_text_properties_t) {
    .flags = CIG_TEXT_FORMATTED
  }, "Roll out the <color=%x>red carpet", &red_color);
  
  TEST_ASSERT_NULL(spans.info[0].color);
  TEST_ASSERT_EQUAL_PTR(&red_color, spans.info[3].color); /* Override */
  TEST_ASSERT_EQUAL_PTR(&red_color, spans.info[4].color); /* Override */
}

TEST(text_style, unknown_tag) {  
  begin();
  
  cig_label((cig_text_properties_t) {
    .flags = CIG_TEXT_FORMATTED
  }, "<rainbow>Sun's out</rainbow>", &red_color);
  
  TEST_ASSERT_EQUAL_STRING("Sun's", spans.info[0].str);
  TEST_ASSERT_EQUAL_STRING("out", spans.info[1].str);
}

TEST_GROUP_RUNNER(text_style) {
  RUN_TEST_CASE(text_style, font_override);
  RUN_TEST_CASE(text_style, color_override);
  RUN_TEST_CASE(text_style, bold);
  RUN_TEST_CASE(text_style, italic);
  RUN_TEST_CASE(text_style, underline);
  RUN_TEST_CASE(text_style, strikethrough);
  RUN_TEST_CASE(text_style, unclosed_tag);
  RUN_TEST_CASE(text_style, unknown_tag);
}