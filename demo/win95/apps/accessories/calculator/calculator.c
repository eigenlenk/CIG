#include "calculator.h"
#include "components/button.h"
#include "components/menu.h"

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <float.h>

typedef enum {
  MEMORY_CLEAR,
  MEMORY_RECALL,
  MEMORY_STORE,
  MEMORY_ADD
} memory_operation;

typedef enum {
  CLEAR_ENTRY = CIG_BIT(0),
  CLEAR_ACCUMULATOR = CIG_BIT(1),
  CLEAR_OPERATION = CIG_BIT(2),
  CLEAR_MEMORY = CIG_BIT(3),
  CLEAR_LAST_ENTRY = CIG_BIT(4),
  CLEAR_LAST_ENTRY_CONDITIONAL = CIG_BIT(5),
  CLEAR_ALL = CLEAR_ENTRY | CLEAR_ACCUMULATOR | CLEAR_OPERATION | CLEAR_MEMORY | CLEAR_LAST_ENTRY
} clear_operation;

typedef enum {
  MATH_NOOP = 0,
  MATH_ADD,
  MATH_SUBTRACT,
  MATH_MULTIPLY,
  MATH_DIVIDE,
} math_operation;

typedef enum {
  DISPLAY_ENTRY,
  DISPLAY_ACCUMULATOR
} display_mode;

typedef struct {
  display_mode display_mode;
  double entry, last_entry, accumulator, memory;
  int decimal_places;
  math_operation operation, repeat_operation;
  bool has_entry, user_entry, in_decimal_entry, percentage, has_memory;
  char display[32];
} app_state;

static void calculator_window_proc(window_t*, bool);
static void memory_buttons(app_state*);
static void clear_buttons(app_state*);
static void numeric_function_keypad(app_state*);
static void perform_memory_operation(app_state*, memory_operation);
static void perform_clear_operation(app_state*, clear_operation);
static void enter_number(app_state*, int);
static void remove_last_number(app_state*);
static void number_button(app_state*, int);
static void app_state_update_display(app_state*);
static void app_state_perform_current_operation(app_state*);
static void app_state_reprocicate(app_state*);
static void app_state_sqrt(app_state*);
static void app_state_flip_sign(app_state*);
static void app_state_set_or_calculate_percentage(app_state*);

application_t
calculator_app()
{
  app_state *state = malloc(sizeof(app_state));
  state->display_mode = DISPLAY_ENTRY;
  state->entry = 0;
  state->last_entry = 0;
  state->accumulator = 0;
  state->memory = 0;
  state->has_entry = false;
  state->user_entry = false;
  state->percentage = false;
  state->has_memory = false;
  state->in_decimal_entry = 0;
  state->decimal_places = 0;
  state->operation = MATH_NOOP;
  app_state_update_display(state);

  return (application_t) {
    .id = "calculator",
    .windows = {
      (window_t) {
        .id = cig_hash("calculator"),
        .proc = &calculator_window_proc,
        .data = NULL,
        .rect = CENTER_APP_WINDOW(276, 272),
        .title = "Calculator",
        .icon = IMAGE_CALCULATOR_16,
        .flags = IS_PRIMARY_WINDOW
      }
    },
    .data = state,
    .flags = KILL_WHEN_PRIMARY_WINDOW_CLOSED
  };
}

static void
calculator_menubar(window_t *wnd)
{
  static win95_menu menu_edit;

  menu_setup(&menu_edit, "Edit", DROPDOWN, NULL, 1, (menu_group[]) {
    {
      .items = {
        .count = 2,
        .list = {
          { .title = "Copy" },
          { .title = "Paste" }
        }
      }
    }
  });

  menubar(1, (win95_menu*[]) { &menu_edit });
}

static void
calculator_window_proc(window_t *this, bool window_focused)
{
  app_state *state = (app_state *)this->owner->data;
  cig_frame *container = cig_current();
  cig_frame *separator, *input_field;

  CIG_RETAIN(separator, CIG(BUILD_RECT(
    PIN(LEFT_OF(container), OFFSET_BY(1)),
    PIN(RIGHT_OF(container), OFFSET_BY(1)),
    PIN(TOP_OF(container), OFFSET_BY(24)),
    PIN(HEIGHT, 2)
  )) {
    cig_fill_style(get_style(STYLE_INNER_BEVEL_NO_FILL), 0);
  });

  CIG(BUILD_RECT(
    PIN(BELOW(separator), OFFSET_BY(2)),
    PIN(LEFT),
    PIN(RIGHT),
    PIN(BOTTOM)
  ), CIG_INSETS(cig_i_uniform(11))) {
    // cig_fill_color(get_color(COLOR_DEBUG));

    /* Input field */
    CIG_RETAIN(input_field, CIG(_H(26), CIG_INSETS(cig_i_horizontal(9))) {
      cig_fill_color(get_color(COLOR_WHITE));
      cig_fill_style(get_style(STYLE_FILES_CONTENT_BEVEL), 0);
      cig_draw_label((cig_text_properties) {
        .alignment.horizontal = CIG_TEXT_ALIGN_RIGHT
      }, state->display);
    });

    CIG_HSTACK(BUILD_RECT(
      PIN(BELOW(input_field), OFFSET_BY(6)),
      PIN(LEFT_INSET),
      PIN(RIGHT_INSET),
      PIN(BOTTOM_INSET)
    ), CIG_PARAMS({
      CIG_SPACING(16)
    })) {
      memory_buttons(state);
      
      CIG_VSTACK(_, CIG_PARAMS({
        CIG_SPACING(6),
      })) {
        CIG_HSTACK(_H(28), CIG_PARAMS({
          CIG_ALIGNMENT_HORIZONTAL(CIG_LAYOUT_ALIGNS_RIGHT),
          CIG_SPACING(4),
          CIG_WIDTH(49)
        })) {
          clear_buttons(state);
        }

        CIG_GRID(_, CIG_PARAMS({
          CIG_SPACING_HORIZONTAL(4),
          CIG_SPACING_VERTICAL(6),
          CIG_COLUMNS(5),
          CIG_ROWS(4)
        })) {
          numeric_function_keypad(state);
        }
      }
    }
  }

  CIG(BUILD_RECT(
    PIN(LEFT_OF(container)),
    PIN(RIGHT_OF(container)),
    PIN(HEIGHT, 18),
    PIN(TOP_OF(container))
  )) {
    calculator_menubar(this);
  }
}

/* PRIVATE METHODS */

static void
memory_buttons(app_state *state)
{
  CIG_VSTACK(_W(36), CIG_PARAMS({
    CIG_SPACING(6),
    CIG_ROWS(5)
  })) {
    CIG(_H(28)) {
      cig_fill_style(get_style(STYLE_FILES_CONTENT_BEVEL), 0);

      if (state->has_memory) {
        cig_draw_label((cig_text_properties) { }, "M");
      }
    }

    button_font = get_font(FONT_BOLD);
    button_title_color = get_color(COLOR_RED);

    if (standard_button(_, "MC")) perform_memory_operation(state, MEMORY_CLEAR);
    if (standard_button(_, "MR")) perform_memory_operation(state, MEMORY_RECALL);
    if (standard_button(_, "MS")) perform_memory_operation(state, MEMORY_STORE);
    if (standard_button(_, "M+")) perform_memory_operation(state, MEMORY_ADD);

    button_font = NULL;
    button_title_color = NULL;
  }
}

static void
clear_buttons(app_state *state)
{
  button_font = get_font(FONT_BOLD);
  button_title_color = get_color(COLOR_MAROON);

  if (standard_button(_, "C")) perform_clear_operation(state, CLEAR_ALL);
  if (standard_button(_, "CE")) perform_clear_operation(state, CLEAR_ENTRY | CLEAR_LAST_ENTRY_CONDITIONAL);
  if (standard_button(_, "Back")) remove_last_number(state);

  button_font = NULL;
  button_title_color = NULL;
}

static void
numeric_function_keypad(app_state *state)
{
  button_font = get_font(FONT_BOLD);

  button_title_color = get_color(COLOR_BLUE);
  number_button(state, 7);
  number_button(state, 8);
  number_button(state, 9);

  button_title_color = get_color(COLOR_RED);
  if (standard_button(_, "/")) {
    if (state->has_entry) {
      state->accumulator = state->entry;
    }
    perform_clear_operation(state, CLEAR_ENTRY);
    state->percentage = false;
    state->operation = MATH_DIVIDE;
    state->display_mode = DISPLAY_ACCUMULATOR;
    app_state_update_display(state);
  }

  button_title_color = get_color(COLOR_NAVY);
  if (standard_button(_, "sqrt")) { app_state_sqrt(state); }

  button_title_color = get_color(COLOR_BLUE);
  number_button(state, 4);
  number_button(state, 5);
  number_button(state, 6);

  button_title_color = get_color(COLOR_RED);
  if (standard_button(_, "*")) {
    if (state->has_entry) {
      state->accumulator = state->entry;
    }
    perform_clear_operation(state, CLEAR_ENTRY);
    state->percentage = false;
    state->operation = MATH_MULTIPLY;
    state->display_mode = DISPLAY_ACCUMULATOR;
    app_state_update_display(state);
  }

  button_title_color = get_color(COLOR_NAVY);
  if (standard_button(_, "%")) { app_state_set_or_calculate_percentage(state); }

  button_title_color = get_color(COLOR_BLUE);
  number_button(state, 1);
  number_button(state, 2);
  number_button(state, 3);

  button_title_color = get_color(COLOR_RED);
  if (standard_button(_, "-")) {
    if (state->has_entry) {
      state->accumulator = state->entry;
    }
    perform_clear_operation(state, CLEAR_ENTRY);
    state->percentage = false;
    state->operation = MATH_SUBTRACT;
    state->display_mode = DISPLAY_ACCUMULATOR;
    app_state_update_display(state);
  }


  button_title_color = get_color(COLOR_NAVY);
  if (standard_button(_, "1/x")) { app_state_reprocicate(state); }

  button_title_color = get_color(COLOR_BLUE);
  number_button(state, 0);
  
  if (standard_button(_, "+/-")) { app_state_flip_sign(state); }
  
  if (standard_button(_, ".")) {
    if (!state->in_decimal_entry) {
      state->in_decimal_entry = true;
      state->decimal_places = 0;
      /* Previous operation is cleared when starting to enter a new number */
      if (state->operation == MATH_NOOP) {
        state->repeat_operation = MATH_NOOP;
      }
    }
  }

  button_title_color = get_color(COLOR_RED);
  
  if (standard_button(_, "+")) {
    if (state->has_entry) {
      state->accumulator = state->entry;
    }
    perform_clear_operation(state, CLEAR_ENTRY);
    state->percentage = false;
    state->operation = MATH_ADD;
    state->display_mode = DISPLAY_ACCUMULATOR;
    app_state_update_display(state);
  }

  if (standard_button(_, "=")) { app_state_perform_current_operation(state); }

  button_title_color = NULL;
  button_font = NULL;
}

/* [[ OPERATIONS ]] */

static void
perform_memory_operation(app_state *state, memory_operation op)
{
  switch (op) {
  case MEMORY_CLEAR:
    state->memory = 0;
    state->has_memory = false;
    break;
  case MEMORY_RECALL:
    if (state->has_memory) {
      state->has_entry = true;
      state->user_entry = false;
      state->last_entry = state->entry = state->memory;
      state->display_mode = DISPLAY_ENTRY;
      app_state_update_display(state);
    }
    break;
  case MEMORY_STORE:
  case MEMORY_ADD:
    {
      double n = state->display_mode == DISPLAY_ENTRY
        ? (state->has_entry ? state->entry : state->last_entry)
        : state->accumulator;
      state->has_memory = true;
      if (op == MEMORY_ADD) {
        state->memory += n;
      } else {
        state->memory = n;
      }
      break;
    }
  }
}

static void
perform_clear_operation(app_state *state, clear_operation op)
{
  if (op & CLEAR_ENTRY) {
    printf("CLEAR_ENTRY\n");
    if (state->has_entry && op & CLEAR_LAST_ENTRY_CONDITIONAL) {
      printf("\talso clears last entry\n");
      state->last_entry = 0;
      state->operation = MATH_NOOP;
    }
    state->entry = 0;
    state->has_entry = false;
    state->user_entry = false;
    state->in_decimal_entry = false;
    state->decimal_places = 0;
    state->display_mode = DISPLAY_ENTRY;
  }

  if (op & CLEAR_ACCUMULATOR) {
    state->accumulator = 0;
  }

  if (op & CLEAR_OPERATION) {
    printf("CLEAR_OPERATION\n");
    state->operation = MATH_NOOP;
    state->repeat_operation = MATH_NOOP;
  }

  if (op & CLEAR_LAST_ENTRY) {
    printf("CLEAR_LAST_ENTRY\n");
    state->last_entry = 0;
  }

  app_state_update_display(state);
}

/* [[ NUMBER ENTRY ]] */

static void
enter_number(app_state *state, int n)
{
  state->has_entry = true;

  if (!state->user_entry) {
    state->entry = 0;
    state->in_decimal_entry = false;
    state->decimal_places = 0;
    state->user_entry = true;
  }

  if (state->in_decimal_entry) {
    state->entry += n / pow(10, ++state->decimal_places);
  } else {
    state->entry = state->entry * 10 + n;
  }

  /* Previous operation is cleared when starting to enter a new number */
  if (state->operation == MATH_NOOP) {
    state->repeat_operation = MATH_NOOP;
  }

  state->last_entry = state->entry;
  state->display_mode = DISPLAY_ENTRY;
  app_state_update_display(state);
}

static void
remove_last_number(app_state *state)
{
  if (!state->user_entry) {
    return;
  }

  if (state->decimal_places > 0) {
    double p = pow(10, --state->decimal_places);
    state->entry = floor(state->entry * p) / p;
  } else {
    state->in_decimal_entry = false;
    state->entry = floor(state->entry / 10);
  }

  state->last_entry = state->entry;
  state->display_mode = DISPLAY_ENTRY;
  app_state_update_display(state);
}

static void
number_button(app_state *state, int n)
{
  const char *numbers[] = {"0","1","2","3","4","5","6","7","8","9"};

  if (standard_button(_, numbers[n])) {
    enter_number(state, n);
  }
}

static void
app_state_update_display(app_state *this)
{
  const double n = this->display_mode == DISPLAY_ENTRY
    ? this->entry
    : this->accumulator;

  double i;
  double f = modf(n, &i);

  if (fabs(f) <= FLT_EPSILON && !this->decimal_places) {
    /* Quirk: Win 95/98/ME shows dot after number to indiate it's floating point capable */
    snprintf(this->display, 32, "%.0lf.", i);
  } else {
    if (!this->decimal_places) {
      snprintf(this->display, 32, "%lg", n);
    } else {
      snprintf(this->display, 32, "%.*f", this->decimal_places, n);
    }
  }
}

static void
app_state_perform_current_operation(app_state *this)
{
  const math_operation op = this->operation != MATH_NOOP
    ? this->operation
    : this->repeat_operation;

  double value;
  if (this->percentage) {
    value = this->accumulator * (this->last_entry / 100.0);
  } else {
    value = this->has_entry ? this->entry : this->last_entry;
  }

  switch (op) {
  case MATH_NOOP:
    if (this->has_entry) {
      this->accumulator = this->entry;
    }
    break;
  case MATH_ADD:
    printf("%lf + %lf\n", this->accumulator, value);
    this->accumulator += value;
    break;
  case MATH_SUBTRACT:
    printf("%lf - %lf\n", this->accumulator, value);
    this->accumulator -= value;
    break;
  case MATH_MULTIPLY:
    printf("%lf * %lf\n", this->accumulator, value);
    this->accumulator *= value;
    break;
  case MATH_DIVIDE:
    printf("%lf / %lf\n", this->accumulator, value);
    this->accumulator /= value;
    break;
  }

  printf("\tAccumulator: %lf\n", this->accumulator);

  perform_clear_operation(this, CLEAR_ENTRY);
  
  if (this->operation != MATH_NOOP) {
    this->repeat_operation = this->operation;
    this->operation = MATH_NOOP;
  }

  this->display_mode = DISPLAY_ACCUMULATOR;
  app_state_update_display(this);
}

static void
app_state_reprocicate(app_state *this)
{
  if (this->has_entry) {
    this->entry = 1 / this->entry;
    this->last_entry = this->entry;
  } else {
    this->accumulator = 1 / this->accumulator;
    this->last_entry = this->accumulator;
  }

  app_state_update_display(this);
}

static void
app_state_sqrt(app_state *this)
{
  if (this->has_entry) {
    this->entry = sqrt(this->entry);
    this->last_entry = this->entry;
  } else {
    this->accumulator = sqrt(this->accumulator);
    this->last_entry = this->accumulator;
  }

  app_state_update_display(this);
}

static void
app_state_flip_sign(app_state *this)
{
  if (this->has_entry) {
    this->entry = -this->entry;
    this->last_entry = this->entry;
  } else {
    this->accumulator = -this->accumulator;
    this->last_entry = this->accumulator;
  }

  app_state_update_display(this);
}

static void
app_state_set_or_calculate_percentage(app_state *this)
{
  this->percentage = true;

  if (this->repeat_operation != MATH_NOOP) {
    app_state_perform_current_operation(this);
  }
}
