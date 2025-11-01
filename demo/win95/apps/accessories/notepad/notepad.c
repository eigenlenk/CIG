#include "notepad.h"
#include "components/scroller.h"

#include <string.h>
#include <stdio.h>

#define MAX_FILE_BUFFER 64*1024
#define MAX_SPANS 64

typedef struct {
  char buffer[MAX_FILE_BUFFER];
  utf8_string ustr;
  const char *head;
  cig_label *label;
  cig_font_info_st font_info;
  bool recalculate_lines;
  int line_count, previous_line_offset, visible_lines_count;
} app_state;

static void
window_proc(window_t *this, bool window_focused)
{
  app_state *state = (app_state *)this->data;
  cig_frame *window_content_area = cig_current();

  CIG(
    BUILD_RECT(
      PIN(LEFT_OF(window_content_area)),
      PIN(RIGHT_OF(window_content_area)),
      PIN(TOP_OF(window_content_area), OFFSET_BY(18)),
      PIN(BOTTOM_OF(window_content_area))
    ),
    CIG_INSETS(cig_i_uniform(2))
  ) {
    cig_fill_color(get_color(COLOR_WHITE));
    cig_fill_style(get_style(STYLE_FILES_CONTENT_BEVEL), 0);

    CIG(_) {
      cig_scroll_state_t *scroll = NULL;

      CIG(RECT(0, 0, CIG_W - 16, CIG_H - 16), cig_i_uniform(1)) {
        cig_enable_scroll(NULL);
        scroll = cig_scroll_state();

        const int visible_lines_count = floorf(cig_rect().h / (float)state->font_info.height);
        const int scrollable_lines = CIG_MAX(0, state->line_count - state->visible_lines_count);

        if (visible_lines_count != state->visible_lines_count) {
          int old_line_offset = scroll->offset.y;
          scroll->offset.y = CIG_MIN(state->line_count - visible_lines_count, scroll->offset.y);
          state->head = cig_utf8_string_line_location(state->ustr, state->head, scroll->offset.y - old_line_offset);
          state->recalculate_lines = true;
        }
        else if (state->previous_line_offset != scroll->offset.y) {
          state->head = cig_utf8_string_line_location(state->ustr, state->head, scroll->offset.y - state->previous_line_offset);
          state->recalculate_lines = true;
        }

        if (state->recalculate_lines) {
          cig_label_prepare(
            state->label,
            cig_r_size(cig_rect()),
            (cig_text_properties) {
              .font = get_font(FONT_FIXEDSYS),
              .alignment.horizontal = CIG_TEXT_ALIGN_LEFT,
              .alignment.vertical = CIG_TEXT_ALIGN_TOP,
              .flags = CIG_TEXT_UNIQUE_STRING_POINTER 
                | CIG_TEXT_HORIZONTAL_WRAP_DISABLED
                | CIG_TEXT_VERTICAL_CLIPPING_ENABLED
            },
            state->head
          );

          printf("Spans: %d, lines: %d (%d), scroll-y: %d/%d\n", state->label->span_count, state->label->line_count, visible_lines_count, scroll->offset.y, scrollable_lines);

          state->visible_lines_count = visible_lines_count;
          state->recalculate_lines = false;
        }

        state->previous_line_offset = scroll->offset.y;
        scroll->distance.y = scrollable_lines;

        cig_label_draw(state->label);
      }

      display_scrollbars(scroll, (scroller_config) { .flags = SCROLLER_ALWAYS_VISIBLE, .scale = 4, .step = 1 }, NULL);
    }
  }

  if (!(this->flags & IS_MAXIMIZED)) {
    CIG(RECT(CIG_W_INSET-14, CIG_H_INSET-14, 12, 12)) {
      cig_draw_image(get_image(IMAGE_RESIZE_HANDLE), CIG_IMAGE_MODE_TOP_LEFT);
    }
  }
}

static int
load_file_contents(char *buffer, const char *path)
{
  FILE *file;

  if ((file = fopen(path, "r"))) {
    size_t len = 0;
    int lines = 0;
    while (fgets(buffer + len, MAX_FILE_BUFFER - len, file)) {
      len += strlen(buffer + len);
      lines++;
    }
    fclose(file);
    buffer[MAX_FILE_BUFFER-1] = '\0';
    return lines;
  } else {
    fprintf(stderr, "Unable to read file: %s\n", path);
  }

  return 0;
}

/* == APPLICATION CLASS == */

application_t
notepad_app()
{
  /* Weird allocation logic to be able to 'free' everything in a single go when closing the app.
     The allocated buffer contains the app state + FAM of text spans as part of the label. */
  size_t total_size = sizeof(app_state) + CIG_LABEL_SIZEOF(MAX_SPANS);
  app_state *state = (app_state *)malloc(total_size);
  memset(state, 0, sizeof(app_state));
  strcpy(state->buffer, "");
  state->label = (cig_label *)((uint8_t *)state + sizeof(app_state));
  state->label->available_spans = MAX_SPANS;
  state->recalculate_lines = true;
  state->font_info = cig_font_info(get_font(FONT_FIXEDSYS));
  state->line_count = load_file_contents(state->buffer, "res/readme.txt");
  state->ustr = make_utf8_string(state->buffer);
  state->head = state->buffer;

  printf("Txt file line count: %d\n", state->line_count);

  return (application_t) {
    .id = "notepad",
    .windows = {
      (window_t) {
        .id = cig_hash("notepad"),
        .proc = &window_proc,
        .data = state,
        .rect = CENTER_APP_WINDOW(400, 300),
        .min_size = { 220, 80 },
        .title = "Readme - Notepad",
        .icon = IMAGE_NOTEPAD_16,
        .flags = IS_PRIMARY_WINDOW | IS_RESIZABLE
      }
    },
    .flags = KILL_WHEN_PRIMARY_WINDOW_CLOSED
  };
}
