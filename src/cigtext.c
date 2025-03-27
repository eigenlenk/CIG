#include "cigtext.h"
#include "utf8.h"

#include <stdio.h>

#define WORD_SPACING 4

typedef struct {
  utf8_string slice;
  cig_vec2_t bounds;
} span_t;

typedef struct {
  cig_id_t hash;
  span_t spans[CIG_LABEL_SPANS_MAX];
  size_t span_count;
} label_t;

static cig_text_render_callback_t render_callback = NULL;
static cig_text_measure_callback_t measure_callback = NULL;
static cig_font_query_callback_t font_query = NULL;
static cig_font_ref default_font = NULL;

static void render_spans(span_t*, size_t);

void cig_set_text_render_callback(cig_text_render_callback_t callback) {
  render_callback = callback;
}

void cig_set_text_measure_callback(cig_text_measure_callback_t callback) {
  measure_callback = callback;
}

void cig_set_font_query_callback(cig_font_query_callback_t callback) {
  font_query = callback;
}

void cig_set_default_font(cig_font_ref font_ref) {
  default_font = font_ref;
}

void cig_label(const char *text, cig_text_horizontal_alignment_t h_alignment) {
  label_t *label = CIG_ALLOCATE(label_t);

  cig_id_t hash = cig_hash(text);
  
  if (label->hash != hash) {
    label->hash = hash;
    label->span_count = 0;
    
    utf8_string utext = make_utf8_string(text);
    utf8_char_iter iter = make_utf8_char_iter(utext);
    
    utf8_char ch;
    uint32_t cp;
    size_t span_start = 0, cur = 0;
    while ((ch = next_utf8_char(&iter)).byte_len > 0 && (cp = unicode_code_point(ch))) {
      // printf("character: %.*s\t", (int)ch.byte_len, ch.str);
      // printf("unicode code point: U+%04X\n", cp);
      
      /* Span terminated: basic space character or end of string */
      if (cp == 0x20 || (cur + ch.byte_len == utext.byte_len)) {
        if (cur + ch.byte_len == utext.byte_len) { cur += ch.byte_len; }
        utf8_string slice = slice_utf8_string(utext, span_start, cur - span_start);
        
        label->spans[label->span_count++] = (span_t) { 
          .slice = slice,
          .bounds = measure_callback
            ? measure_callback(slice.str, slice.byte_len)
            : cig_vec2_zero()
        };
        
        span_start = cur + ch.byte_len;
      }
      
      cur += ch.byte_len;
    }
  }
  
  if (render_callback) {
    render_spans(&label->spans[0], label->span_count);
  }
}

/* ╔════════════════════════════════════════════╗
   ║            INTERNAL FUNCTIONS              ║
   ╚════════════════════════════════════════════╝ */

static void render_spans(span_t *first, size_t count) {
  register const cig_rect_t absolute_rect = cig_absolute_rect();
  register int lines, x, y, w, dx, dy;
  register span_t *span, *line_start, *line_end, *last = first + count;
  
  register const cig_font_info_t font_info = font_query(default_font);
  
  /* Figure out number of lines needed */
  for (span = first, x = 0, y = 0; span < last; span++) {
    if (x + span->bounds.x > absolute_rect.w && x > 0) {
      x = 0;
      y ++;
    }
    
    x += span->bounds.x + WORD_SPACING;
  }
  
  lines = 1 + y;
  dy = absolute_rect.y + (int)(CIG_H - (lines * font_info.height)) / 2;

  for (line_start = span = first, x = 0; span < last;) {
    if (x + span->bounds.x > absolute_rect.w) {
      if (!x) {
        w = span->bounds.x;
        line_end = span+1;
      } else {
        line_end = span;
        w = x - WORD_SPACING;
        x = 0;
      }
    } else if (span == last-1) {
      line_end = last;
      w = x + span->bounds.x;
    } else {
      line_end = NULL;
      x += span->bounds.x + WORD_SPACING;
    }
    
    if (line_end) {
      dx = absolute_rect.x + (int)((CIG_W - w) * 0.5);
      
      for (span = line_start; span < line_end; dx += (span++)->bounds.x + WORD_SPACING) {
        render_callback(
          cig_rect_make(dx, dy, span->bounds.x, span->bounds.y),
          span->slice.str,
          span->slice.byte_len
        );
      }

      span = line_start = line_end;
      dy += font_info.height;
      continue;
    }
    
    span++;
  }
}

















