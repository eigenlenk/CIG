#include "cigtext.h"
#include "utf8.h"

#include <stdio.h>

#define WORD_SPACING 4

typedef struct {
  utf8_string slice;
  struct { unsigned short w, h; } bounds;
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

static void render_spans(span_t*, size_t, cig_text_properties_t*);

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

void cig_label(const char *text, cig_text_properties_t props) {
  props.font = props.font ? props.font : default_font;
    
  cig_id_t hash = cig_hash(text);
  
  label_t *label = CIG_ALLOCATE(label_t);

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
        cig_vec2_t bounds = measure_callback
          ? measure_callback(slice.str, slice.byte_len, props.font)
          : cig_vec2_zero();
        
        label->spans[label->span_count++] = (span_t) { 
          .slice = slice,
          .bounds = { bounds.x, bounds.y }
        };
        
        span_start = cur + ch.byte_len;
      }
      
      cur += ch.byte_len;
    }
  }
  
  if (render_callback) {
    render_spans(&label->spans[0], label->span_count, &props);
  }
}

/* ╔════════════════════════════════════════════╗
   ║            INTERNAL FUNCTIONS              ║
   ╚════════════════════════════════════════════╝ */

static void render_spans(span_t *first, size_t count, cig_text_properties_t *props) {
  register const cig_rect_t absolute_rect = cig_rect_inset(cig_absolute_rect(), cig_frame()->insets);
  register int lines, x, y, w, dx, dy;
  register span_t *span, *line_start, *line_end, *last = first + count;
  
  register const cig_font_info_t font_info = font_query(props->font);
  
  /* Figure out number of lines needed */
  for (span = first, x = 0, y = 0; span < last; span++) {
    if (x + span->bounds.w > absolute_rect.w && x > 0) {
      x = 0;
      y ++;
    }
    
    x += span->bounds.w + WORD_SPACING;
  }
  
  lines = 1 + y;
  
  switch (props->alignment.vertical) {
    case CIG_TEXT_ALIGN_TOP: { dy = absolute_rect.y; } break;
    case CIG_TEXT_ALIGN_MIDDLE: { dy = absolute_rect.y + (int)(absolute_rect.h - (lines * font_info.height)) * 0.5; } break;
    case CIG_TEXT_ALIGN_BOTTOM: { dy = absolute_rect.y + absolute_rect.h - (lines * font_info.height); } break;
  }

  for (line_start = span = first, x = 0; span < last;) {
    if (x + span->bounds.w> absolute_rect.w) {
      if (!x) {
        w = span->bounds.w;
        line_end = span+1;
      } else {
        line_end = span;
        w = x - WORD_SPACING;
        x = 0;
      }
    } else if (span == last-1) {
      line_end = last;
      w = x + span->bounds.w;
    } else {
      line_end = NULL;
      x += span->bounds.w + WORD_SPACING;
    }
    
    if (line_end) {
      switch (props->alignment.horizontal) {
        case CIG_TEXT_ALIGN_LEFT: dx = absolute_rect.x; break;
        case CIG_TEXT_ALIGN_CENTER: dx = absolute_rect.x + (int)((absolute_rect.w - w) * 0.5); break;
        case CIG_TEXT_ALIGN_RIGHT: dx = absolute_rect.x + absolute_rect.w - w; break;
      }
      
      for (span = line_start; span < line_end; dx += (span++)->bounds.w + WORD_SPACING) {
        render_callback(
          cig_rect_make(dx, dy, span->bounds.w, span->bounds.h),
          span->slice.str,
          span->slice.byte_len,
          props->font,
          props->color
        );
      }

      span = line_start = line_end;
      dy += font_info.height;
      continue;
    }
    
    span++;
  }
}

















