#include "cigcore.h"
#include "cigcorem.h"
#include <string.h>
#include <assert.h>

static cig_context_t *current = NULL;
static cig_set_clip_rect_callback_t set_clip = NULL;

#ifdef DEBUG
static cig_layout_breakpoint_callback_t layout_breakpoint_callback = NULL;
static bool requested_layout_step_mode = false;
#endif

/*  Forward delcarations */
static CIG_OPTIONAL(cig_state_t*) find_state(cig_id_t);
static CIG_OPTIONAL(cig_scroll_state_t*) find_scroll_state(cig_id_t);
static void handle_frame_hover(const cig_frame_t*);
static void push_clip(cig_frame_t*);
static void pop_clip();
static cig_r resolve_size(cig_r, const cig_frame_t*);
static bool next_layout_rect(cig_r, cig_frame_t*, cig_r*);
static bool push_frame(cig_r, cig_i, cig_layout_params_t, bool (*)(cig_r, cig_r, cig_layout_params_t*, cig_r*));

CIG_INLINED int tinyhash(int a, int b) {
  return (a * 31) ^ (b * 17);
}

CIG_INLINED int limit(int v, const int minv_or_zero, const int maxv_or_zero) {
  if (maxv_or_zero > 0) { v = CIG_MIN(maxv_or_zero, v); }
  if (minv_or_zero > 0) { v = CIG_MAX(minv_or_zero, v); }
  return v;
}

/*  Is CIG__AUTO_BIT set? For negative numbers we invert the mask because two's complement */
CIG_INLINED bool is_auto(int32_t n) {
  return (n < 0 ? n & ~CIG__AUTO_BIT : n) & CIG__AUTO_BIT;
}

/*  Is CIG__REL_BIT set? For negative numbers we invert the mask because two's complement */
CIG_INLINED bool is_rel(int32_t n) {
  return (n < 0 ? n & ~CIG__REL_BIT : n) & CIG__REL_BIT;
}

/*  Clear option bits and get either absolute value, AUTO value or REL value */
CIG_INLINED int get_value(int32_t n, int relating_to) {
  register bool rel = is_rel(n);
  if (rel || is_auto(n)) {
    register int v = n < 0 ? (n | CIG__AUTO_BIT | CIG__REL_BIT) : ((n&~CIG__AUTO_BIT)&~CIG__REL_BIT);
    if (rel) {
      return (v/100000.0)*relating_to;
    } else {
      return relating_to+v;
    }
  }
  return n;
}

/*  ┌─────────────┐
    │ CORE LAYOUT │
    └─────────────┘ */

void cig_init_context(cig_context_t *context) {
  context->frames = INIT_STACK(cig_frame_t);
  context->buffers = INIT_STACK(cig_buffer_element_t);
  context->input_state = (cig_input_state_t) { 0 };
  context->next_id = 0;
  context->tick = 1;
  context->delta_time = 0.f;
  context->elapsed_time = 0.f;
  
  for (int i = 0; i < CIG_STATES_MAX; ++i) {
    context->state_list[i].id = 0;
    context->state_list[i].last_tick = context->tick;
  }
  
  for (int i = 0; i < CIG_SCROLLABLE_ELEMENTS_MAX; ++i) {
    context->scroll_elements[i].id = 0;
    context->scroll_elements[i].last_tick = context->tick;
  }
}

void cig_begin_layout(
  cig_context_t *context,
  const cig_buffer_ref buffer,
  const cig_r rect,
  const float delta_time
) {
  current = context;

  current->frames.clear(&current->frames);
  current->buffers.clear(&current->buffers);
  current->delta_time = delta_time;
  current->elapsed_time += delta_time;
  current->default_insets = cig_i_zero();

#ifdef DEBUG
  if (requested_layout_step_mode && current->step_mode == false) {
    requested_layout_step_mode = false;
    current->step_mode = true;
  } else if (current->step_mode) {
    current->step_mode = false;
  }
#endif

  current->frames.push(&current->frames, (cig_frame_t) {
    .id = current->next_id ? current->next_id : cig_hash("root"),
    .rect = cig_r_make(0, 0, rect.w, rect.h),
    .clipped_rect = rect,
    .absolute_rect = rect,
    .insets = cig_i_zero(),
    ._layout_function = NULL,
    ._layout_params = (cig_layout_params_t) { 0 }
  });

#ifdef DEBUG
  cig_trigger_layout_breakpoint(cig_r_zero(), cig_r_make(0, 0, rect.w, rect.h));
#endif

  cig_push_buffer(buffer);
  // cig_enable_clipping();
  current->next_id = 0;
}

void cig_end_layout() {
  for (register int i = 0; i < CIG_STATES_MAX; ++i) {
    if (current->state_list[i].last_tick == current->tick) {
      if (current->state_list[i].value.activation_state == ACTIVATED) {
        current->state_list[i].value.activation_state = ACTIVE;
      }
    } else {
      current->state_list[i].value.activation_state = INACTIVE;
    }
  }

  ++current->tick;
}

cig_buffer_ref cig_buffer() {
  return current->buffers._peek(&current->buffers, 0)->buffer;
}

bool cig_push_frame_args(cig_frame_args_t args) {
  return push_frame(args.rect, args.insets, args.params, args.builder);
}

bool cig_push_frame(const cig_r rect) {
  return push_frame(rect, current->default_insets, (cig_layout_params_t){ 0 }, NULL);
}

bool cig_push_frame_insets(const cig_r rect, const cig_i insets) {
  return push_frame(rect, insets, (cig_layout_params_t){ 0 }, NULL);
}

bool cig_push_frame_insets_params(const cig_r rect, const cig_i insets, const cig_layout_params_t params) {
  return push_frame(rect, insets, params, NULL);
}

bool cig_push_layout_function(
  bool (*layout_function)(const cig_r, const cig_r, cig_layout_params_t *, cig_r *),
  const cig_r rect,
  const cig_i insets,
  cig_layout_params_t params
) {
  return push_frame(rect, insets, params, layout_function);
}

cig_frame_t* cig_pop_frame() {
  cig_frame_t *popped_frame = stack_cig_frame_t__pop(cig_frame_stack());

  if (popped_frame->_clipped) {
    popped_frame->_clipped = false;
    pop_clip();
  }

  return popped_frame;
}

void cig_set_default_insets(cig_i insets) {
  current->default_insets = insets;
}

cig_frame_t* cig_frame() {
  return stack_cig_frame_t__peek(cig_frame_stack(), 0);
}

cig_r cig_convert_relative_rect(const cig_r rect) {
  const cig_buffer_element_t *buffer_element = current->buffers._peek(&current->buffers, 0);
  const cig_frame_t *frame = cig_frame();
  
  return cig_r_offset(
    resolve_size(rect, frame),
    frame->absolute_rect.x + frame->insets.left - buffer_element->origin.x,
    frame->absolute_rect.y + frame->insets.top - buffer_element->origin.y
  );
}

cig_frame_t_stack_t* cig_frame_stack() {
  return &current->frames;
}

/*  ┌───────┐
    │ STATE │
    └───────┘ */

CIG_OPTIONAL(cig_state_t*) cig_state() {
  cig_frame_t *frame = cig_frame();
  if (frame->_state) {
    return frame->_state;
  }
  if ((frame->_state = find_state(frame->id))) {
    frame->_state->arena.mapped = 0;
  }
  return frame->_state;
}

CIG_OPTIONAL(void*) cig_state_allocate(size_t bytes) {
  cig_state_t *state = cig_state();
  
  if (!state) {
    return NULL;
  } else if (state->arena.mapped + bytes >= CIG_STATE_MEM_ARENA_BYTES) {
    return NULL;
  }

  void *result = &state->arena.bytes[state->arena.mapped];
  state->arena.mapped += bytes;
  return result;
}

/*  ┌──────────────────────────────┐
    │ TEMPORARY BUFFERS (ADVANCED) │
    └──────────────────────────────┘ */

void cig_push_buffer(const cig_buffer_ref buffer) {
  cig_buffer_element_t buffer_element = {
    .buffer = buffer,
    .origin = current->buffers.size == 0
      ? cig_v_zero()
      : cig_v_make(cig_frame()->absolute_rect.x, cig_frame()->absolute_rect.y),
     .clip_rects = INIT_STACK(cig_clip_rect_t)
  };

  buffer_element.clip_rects.push(&buffer_element.clip_rects, cig_frame()->absolute_rect);
  current->buffers.push(&current->buffers, buffer_element);
}

void cig_pop_buffer() {
  CIG_UNUSED(current->buffers._pop(&current->buffers));
}

/*  ┌───────────────────────────┐
    │ MOUSE INTERACTION & FOCUS │
    └───────────────────────────┘ */

void cig_set_input_state(
  const cig_v position,
  cig_input_action_type_t action_mask
) {
  // TODO: Make sense and clean up the mess that is this function...

  if (current->input_state.locked == false) {
    if (current->input_state._target_prev_tick != current->input_state._target_this_tick) {
      current->input_state._click_count = 0;
    }
    current->input_state._target_prev_tick = current->input_state._target_this_tick;
    current->input_state._target_this_tick = 0;
  }

  if (current->input_state._focus_target_this > 0) {
    current->input_state._focus_target = current->input_state._focus_target_this;
    current->input_state._focus_target_this = 0;
  }

  const cig_input_action_type_t previous_action_mask = current->input_state.action_mask;

  current->input_state.position = position;
  current->input_state.action_mask = action_mask;

  current->input_state.click_state = (!current->input_state.action_mask && previous_action_mask)
    ? (current->elapsed_time - current->input_state._press_start_time) <= CIG_CLICK_EXPIRE_IN_SECONDS
      ? ENDED
      : EXPIRED
    : (action_mask & CIG_INPUT_MOUSE_BUTTON_LEFT && !(previous_action_mask & CIG_INPUT_MOUSE_BUTTON_LEFT)) || (action_mask & CIG_INPUT_MOUSE_BUTTON_RIGHT && !(previous_action_mask & CIG_INPUT_MOUSE_BUTTON_RIGHT))
      ? current->input_state.click_state == BEGAN
        ? NEITHER
        : BEGAN
      : NEITHER;

  if (current->input_state.action_mask & CIG_INPUT_MOUSE_BUTTON_LEFT && !(previous_action_mask & CIG_INPUT_MOUSE_BUTTON_LEFT)) {
    current->input_state.last_action_began = CIG_INPUT_MOUSE_BUTTON_LEFT;
  } else if (current->input_state.action_mask & CIG_INPUT_MOUSE_BUTTON_RIGHT && !(previous_action_mask & CIG_INPUT_MOUSE_BUTTON_RIGHT)) {
    current->input_state.last_action_began = CIG_INPUT_MOUSE_BUTTON_RIGHT;
  } else {
    current->input_state.last_action_began = 0;
  }

  if (previous_action_mask & CIG_INPUT_MOUSE_BUTTON_LEFT && !(current->input_state.action_mask & CIG_INPUT_MOUSE_BUTTON_LEFT)) {
    current->input_state.last_action_ended = CIG_INPUT_MOUSE_BUTTON_LEFT;
  } else if (previous_action_mask & CIG_INPUT_MOUSE_BUTTON_RIGHT && !(current->input_state.action_mask & CIG_INPUT_MOUSE_BUTTON_RIGHT)) {
    current->input_state.last_action_ended = CIG_INPUT_MOUSE_BUTTON_RIGHT;
  } else {
    current->input_state.last_action_ended = 0;
  }

  switch (current->input_state.click_state) {
  case NEITHER:
    {
      if (current->input_state._click_count && current->elapsed_time - current->input_state._click_end_time > CIG_CLICK_EXPIRE_IN_SECONDS) {
        current->input_state._click_count = 0;
      }
    } break;

  case BEGAN:
    {
      current->input_state._press_start_time = current->elapsed_time;
      current->input_state._press_target_id = current->input_state._target_prev_tick;
    } break;

  case ENDED:
    {
      current->input_state._click_count ++;
      current->input_state._click_end_time = current->elapsed_time;
    } break;

  case EXPIRED:
    {
      current->input_state._click_count = 0;
    } break;
  }

  if (current->input_state.action_mask) {
    if (!current->input_state.drag.active) {
      current->input_state.drag.active = true;
      current->input_state.drag.start_position = current->input_state.position;
      current->input_state.drag.change = cig_v_zero();
    } else {
      current->input_state.drag.change = cig_v_sub(current->input_state.position, current->input_state.drag.start_position);
    }
  } else if (current->input_state.drag.active) {
    current->input_state.drag.active = false;
    current->input_state.drag.change = cig_v_zero();
    current->input_state.locked = false;
  }
}

cig_input_state_t *cig_input_state() {
  return &current->input_state;
}

void cig_enable_interaction() {
  cig_frame_t *frame = cig_frame();
  if (frame->_interaction_enabled) {
    return;
  }
  frame->_interaction_enabled = true;
  handle_frame_hover(frame);
}

bool cig_hovered() {
  /*  See `handle_frame_hover` */
  return current->input_state.locked == false && cig_frame()->_interaction_enabled && cig_frame()->id == current->input_state._target_prev_tick;
}

cig_input_action_type_t cig_pressed(
  const cig_input_action_type_t actions,
  const cig_press_flags_t options
) {
  if (!cig_hovered()) {
    return 0;
  }

  const cig_input_action_type_t action_mask = actions & current->input_state.action_mask;

  if (action_mask && ((options & CIG_PRESS_INSIDE) == false || (options & CIG_PRESS_INSIDE && current->input_state._press_target_id == cig_frame()->id))) {
    return action_mask;
  } else {
    return 0;
  }
}

cig_input_action_type_t cig_clicked(
  const cig_input_action_type_t actions,
  const cig_click_flags_t options
) {
  if (!cig_hovered()) {
    return 0;
  }

  cig_input_action_type_t result;

  if (options & CIG_CLICK_ON_PRESS) {
    if (current->input_state.click_state == BEGAN && (result = actions & current->input_state.action_mask)) {
      return result;
    }
  } else {
    const int required_clicks = options & CIG_CLICK_DOUBLE ? 2 : 1;
    if (
      (current->input_state.click_state == ENDED && current->input_state._click_count == required_clicks) ||
      (current->input_state.click_state == EXPIRED && !(options & CIG_CLICK_EXPIRE) && required_clicks == 1)
    ) {
      if (options & CIG_CLICK_STARTS_INSIDE && current->input_state._press_target_id != cig_frame()->id) {
        return 0;
      }
      if ((result = actions & current->input_state.last_action_ended)) {
        current->input_state._click_count = 0;
        return result;
      }
    }
  }

  return 0;
}

bool cig_enable_focus() {
  const cig_frame_t *frame = cig_frame();

  if (current->input_state.click_state == BEGAN && cig_r_contains(frame->absolute_rect, current->input_state.position)) {
    current->input_state._focus_target_this = frame->id;
  }

  return cig_focused();
}

bool cig_focused() {
  return cig_focused_id() == cig_frame()->id;
}

cig_id_t cig_focused_id() {
  return current->input_state._focus_target;
}

void cig_set_focused_id(cig_id_t id) {
  current->input_state._focus_target = id;
}

/*  ┌───────────┐
    │ SCROLLING │
    └───────────┘ */

bool cig_enable_scroll(cig_scroll_state_t *state) {
  cig_frame_t *frame = cig_frame();
  frame->_scroll_state = state ? state : find_scroll_state(frame->id);
  cig_enable_clipping();
  return frame->_scroll_state != NULL;
}

cig_scroll_state_t* cig_scroll_state() {
  return cig_frame()->_scroll_state;
}

void cig_set_offset(cig_v offset) {
  assert(cig_scroll_state());
  cig_scroll_state()->offset = offset;
}

void cig_change_offset(cig_v delta) {
  assert(cig_scroll_state());
  cig_scroll_state()->offset = cig_v_add(cig_scroll_state()->offset, delta);
}

cig_v cig_offset() {
  assert(cig_scroll_state());
  return cig_scroll_state()->offset;
}

cig_v cig_content_size() {
  assert(cig_scroll_state());
  return cig_scroll_state()->content_size;
}

/*  ┌────────────────┐
    │ LAYOUT HELPERS │
    └────────────────┘ */

void cig_disable_culling() {
  cig_frame()->_layout_params.flags |= CIG_LAYOUT_DISABLE_CULLING;
}

void cig_enable_clipping() {
  push_clip(cig_frame());
}

void cig_set_next_id(cig_id_t id) {
  current->next_id = id;
}

unsigned int cig_depth() {
  return cig_frame_stack()->size;
}

cig_id_t cig_hash(const char *str) {
  /*  http://www.cse.yorku.ca/~oz/hash.html */
  register cig_id_t hash = 5381;
  register int c;
  while ((c = *str++)) {
    hash = ((hash << 5) + hash) + c; /* === hash * 33 + c */
  }
  return hash;
}

void cig_empty() {
  cig_push_frame(RECT_AUTO);
  cig_pop_frame();
}

void cig_spacer(const int size) {
  cig_push_frame(RECT_AUTO_H(size));
  cig_pop_frame();
}

CIG_INLINED void move_to_next_row(cig_layout_params_t *prm) {
  prm->_h_pos = 0;
  prm->_v_pos += (prm->_v_size + prm->spacing);
  prm->_h_size = 0;
  prm->_v_size = 0;
  prm->_count.h_cur = 0;
}

CIG_INLINED void move_to_next_column(cig_layout_params_t *prm) {
  prm->_v_pos = 0;
  prm->_h_pos += (prm->_h_size + prm->spacing);
  prm->_h_size = 0;
  prm->_v_size = 0;
  prm->_count.v_cur = 0;
}

bool cig_default_layout_builder(
  const cig_r container, /* Rect into which sub-frames are laid out */
  const cig_r rect,      /* Proposed rect, generally from CIG_FILL */
  cig_layout_params_t *prm,
  cig_r *result
) {
  const bool h_axis = prm->axis & CIG_LAYOUT_AXIS_HORIZONTAL;
  const bool v_axis = prm->axis & CIG_LAYOUT_AXIS_VERTICAL;
  const bool is_grid = h_axis && v_axis;

  int x = prm->_h_pos, /* TODO: Should these *not* ignore relative offsets? */
      y = prm->_v_pos,
      w,
      h;

  if (h_axis) {
    if (rect.w & CIG__AUTO_BIT) {
      if (prm->width > 0) {
        w = get_value(rect.w, prm->width);
      } else if (prm->columns) {
        w = get_value(rect.w, (container.w - ((prm->columns - 1) * prm->spacing)) / prm->columns);
      } else if (is_grid && prm->_h_size && prm->direction == CIG_LAYOUT_DIRECTION_VERTICAL) {
        w = get_value(rect.w, prm->_h_size);
      } else {
        w = get_value(rect.w, container.w - prm->_h_pos);
      }
    } else {
      w = get_value(rect.w, container.w);
    }
  } else {
    w = get_value(rect.w, (rect.w & CIG__AUTO_BIT) ? container.w - prm->_h_pos : container.w);

    /*  Reset any remaining horizontal positioning in case we modify axis mid-layout */
    prm->_h_pos = 0;
    prm->_h_size = 0;
  }

  if (v_axis) {
    if (rect.h & CIG__AUTO_BIT) {
      if (prm->height > 0) {
        h = get_value(rect.h, prm->height);
      } else if (prm->rows) {
        h = get_value(rect.h, (container.h - ((prm->rows - 1) * prm->spacing)) / prm->rows);
      } else if (is_grid && prm->_v_size && prm->direction == CIG_LAYOUT_DIRECTION_HORIZONTAL) {
        h = get_value(rect.h, prm->_v_size);
      } else {
        h = get_value(rect.h, container.h - prm->_v_pos);
      }
    } else {
      h = get_value(rect.h, container.h);
    }
  } else {
    h = get_value(rect.h, (rect.h & CIG__AUTO_BIT) ? container.h - prm->_v_pos : container.h);

    /*  Reset any remaining vertical positioning in case we modify axis mid-layout */
    prm->_v_pos = 0;
    prm->_v_size = 0;
  }

  w = limit(w, prm->size_min.width, prm->size_max.width);
  h = limit(h, prm->size_min.height, prm->size_max.height);

  if (h_axis && v_axis) {
    /*  Can we fit the new frame onto current axis? */
    switch (prm->direction) {
      case CIG_LAYOUT_DIRECTION_HORIZONTAL: {
        if ((prm->limit.horizontal && prm->_count.h_cur == prm->limit.horizontal) || prm->_h_pos + w > container.w) {
          move_to_next_row(prm);
          x = 0;
          y = prm->_v_pos;
        }
        
        prm->_h_pos += (w + prm->spacing);
        prm->_h_size = CIG_MAX(prm->_h_size, w);
        prm->_v_size = CIG_MAX(prm->_v_size, h);
        prm->_count.h_cur ++;
        
        if (prm->_h_pos >= container.w) {
          move_to_next_row(prm);
        }
      } break;
      case CIG_LAYOUT_DIRECTION_VERTICAL: {
        if ((prm->limit.vertical && prm->_count.v_cur == prm->limit.vertical) || prm->_v_pos + h > container.h) {
          move_to_next_column(prm);
          y = 0;
          x = prm->_h_pos;
        }

        prm->_v_pos += (h + prm->spacing);
        prm->_h_size = CIG_MAX(prm->_h_size, w);
        prm->_v_size = CIG_MAX(prm->_v_size, h);
        prm->_count.v_cur ++;

        if (prm->_v_pos >= container.h) {
          move_to_next_column(prm);
        }
      } break;
      default: break;
    }
  } else if (h_axis) {
    if (prm->limit.horizontal && prm->_count.h_cur == prm->limit.horizontal) {
      return false;
    }
    prm->_h_pos += (w + prm->spacing);
    prm->_h_size = CIG_MAX(prm->_h_size, w);
    prm->_count.h_cur ++;
  } else if (v_axis) {
    if (prm->limit.vertical && prm->_count.v_cur == prm->limit.vertical) {
      return false;
    }
    prm->_v_pos += (h + prm->spacing);
    prm->_v_size = CIG_MAX(prm->_v_size, h);
    prm->_count.v_cur ++;
  }

  switch (prm->alignment.horizontal) {
  case CIG_LAYOUT_ALIGNS_CENTER: { x = (container.w - w) * 0.5; } break;
  case CIG_LAYOUT_ALIGNS_RIGHT: { x = (container.w - (x+w)); } break;
  default: break;
  }

  switch (prm->alignment.vertical) {
  case CIG_LAYOUT_ALIGNS_CENTER: { y = (container.h - h) * 0.5; } break;
  case CIG_LAYOUT_ALIGNS_BOTTOM: { y = (container.h - (y+h)); } break;
  default: break;
  }

  *result = cig_r_make(x, y, w, h);
  
  return true;
}

bool cig_push_hstack(cig_r rect, cig_i insets, cig_layout_params_t params) {
  params.axis = CIG_LAYOUT_AXIS_HORIZONTAL;
  return cig_push_layout_function(&cig_default_layout_builder, rect, insets, params);
}

bool cig_push_vstack(cig_r rect, cig_i insets , cig_layout_params_t params) {
  params.axis = CIG_LAYOUT_AXIS_VERTICAL;
  return cig_push_layout_function(&cig_default_layout_builder, rect, insets, params);
}

bool cig_push_grid(cig_r rect, cig_i insets, cig_layout_params_t params) {
  params.axis = CIG_LAYOUT_AXIS_HORIZONTAL | CIG_LAYOUT_AXIS_VERTICAL;
  return cig_push_layout_function(&cig_default_layout_builder, rect, insets, params);
}

/*  ┌───────────────────┐
    │ BACKEND CALLBACKS │
    └───────────────────┘ */

void cig_set_clip_rect_callback(cig_set_clip_rect_callback_t fp) {
  set_clip = fp;
}

/*  ┌────────────────────┐
    │ INTERNAL FUNCTIONS │
    └────────────────────┘ */

static cig_r resolve_size(const cig_r rect, const cig_frame_t *parent) {
  const cig_r content_rect = cig_r_inset(parent->rect, parent->insets);

  return cig_r_make(
    /*  X & Y components can only have the RELATIVE flag set, AUTO makes
        no sense in this context. They are relative to W & H respectively.
        Eg. X = 25% = W * 0.25 */
    is_rel(rect.x) ? get_value(rect.x, content_rect.w) : rect.x,
    is_rel(rect.y) ? get_value(rect.y, content_rect.h) : rect.y,
    limit(
      get_value(rect.w, content_rect.w),
      parent->_layout_params.size_min.width,
      parent->_layout_params.size_max.width
    ),
    limit(
      get_value(rect.h, content_rect.h),
      parent->_layout_params.size_min.height,
      parent->_layout_params.size_max.height
    )
  );
}

static bool next_layout_rect(const cig_r proposed, cig_frame_t *parent, cig_r *result) {
  if (parent->_layout_function) {
    return (*parent->_layout_function)(
      cig_r_inset(parent->rect, parent->insets),
      proposed,
      &parent->_layout_params,
      result
    );
  } else {
    *result = resolve_size(proposed, parent);
    return true;
  }
}

static bool push_frame(
  const cig_r rect,
  const cig_i insets,
  cig_layout_params_t params,
  bool (*layout_function)(cig_r, cig_r, cig_layout_params_t*, cig_r*)
) {
  cig_buffer_element_t *current_buffer = current->buffers._peek(&current->buffers, 0);
  cig_frame_t *top = cig_frame();

  if (top->_layout_params.limit.total > 0 && top->_layout_params._count.total == top->_layout_params.limit.total) {
    return false;
  }

  cig_r next;
  if (!next_layout_rect(rect, top, &next)) {
    top->_id_counter ++;
    return false;
  }

  if (top->_scroll_state) {
    top->_scroll_state->content_size.x = CIG_MAX(top->_scroll_state->content_size.x, next.x + next.w);
    top->_scroll_state->content_size.y = CIG_MAX(top->_scroll_state->content_size.y, next.y + next.h);
    next = cig_r_offset(next, -top->_scroll_state->offset.x, -top->_scroll_state->offset.y);
  }

  if (!(top->_layout_params.flags & CIG_LAYOUT_DISABLE_CULLING)
    && !cig_r_intersects(top->rect, cig_r_offset(next, top->rect.x+top->insets.left, top->rect.y+top->insets.top))) {
    top->_id_counter ++;
    return false;
  }

  top->_layout_params._count.total ++;

  cig_r absolute_rect = cig_convert_relative_rect(next);
  cig_r current_clip_rect = current_buffer->clip_rects.peek(&current_buffer->clip_rects, 0);

  current->frames.push(&current->frames, (cig_frame_t) {
    .id = current->next_id
      ? current->next_id
      : (top->id + tinyhash(top->id+top->_id_counter++, cig_depth())),
    .rect = next,
    .clipped_rect = cig_r_offset(cig_r_union(absolute_rect, current_clip_rect), -absolute_rect.x + next.x, -absolute_rect.y + next.y),
    .absolute_rect = absolute_rect,
    .insets = insets,
    ._layout_function = layout_function,
    ._layout_params = params,
    ._clipped = false,
    ._scroll_state = NULL
  });

  current->next_id = 0;

#ifdef DEBUG
  cig_trigger_layout_breakpoint(top->absolute_rect, absolute_rect);
#endif

  return true;
}

static void handle_frame_hover(const cig_frame_t *frame) {
  if (cig_r_contains(frame->absolute_rect, current->input_state.position)) {
    current->input_state._target_this_tick = frame->id;
  }
}

static CIG_OPTIONAL(cig_state_t*) find_state(const cig_id_t id) {
  register int open = -1, stale = -1;

  /*  Find a state with a matching ID, with no ID yet, or a stale state */
  for (register int i = 0; i < CIG_STATES_MAX; ++i) {
    if (current->state_list[i].id == id) {
      if (current->state_list[i].value.activation_state == INACTIVE) {
        current->state_list[i].value.activation_state = ACTIVATED;
      }
      current->state_list[i].last_tick = current->tick;
      return &current->state_list[i].value;
    }
    else if (open < 0 && !current->state_list[i].id) {
      open = i;
    }
    else if (stale < 0 && current->state_list[i].last_tick < current->tick-1) {
      stale = i;
    }
  }

  const int result = open >= 0 ? open : stale;

  if (result >= 0) {
    current->state_list[result].id = id;
    current->state_list[result].last_tick = current->tick;
    current->state_list[result].value.activation_state = ACTIVATED;
    memset(&current->state_list[result].value.arena.bytes, 0, CIG_STATE_MEM_ARENA_BYTES);
    current->state_list[result].value.arena.mapped = 0;
    return &current->state_list[result].value;
  }

  return NULL;
}

static CIG_OPTIONAL(cig_scroll_state_t*) find_scroll_state(const cig_id_t id) {
  register int first_unused = -1, first_stale = -1;

  for (register int i = 0; i < CIG_SCROLLABLE_ELEMENTS_MAX; ++i) {
    if (current->scroll_elements[i].id == id) {
      current->scroll_elements[i].last_tick = current->tick;
      return &current->scroll_elements[i].value;
    }
    else if (current->scroll_elements[i].id == 0 && first_unused < 0) {
      first_unused = i;
    }
    else if (current->scroll_elements[i].last_tick < current->tick-1 && first_stale < 0) {
      first_stale = i;
    }
  }

  if (first_unused < 0 && first_stale < 0) {
    return NULL;
  }
  else {
    /*  Prefer an unused slot, use stale when nothing else is available */
    int i = (first_unused >= 0) ? first_unused : first_stale;

    current->scroll_elements[i].id = id;
    current->scroll_elements[i].value.offset = cig_v_zero();
    current->scroll_elements[i].value.content_size = cig_v_zero();
    current->scroll_elements[i].last_tick = current->tick;

    return &current->scroll_elements[i].value;
  }
}

static void push_clip(cig_frame_t *frame) {
  if (frame->_clipped == false) {
    frame->_clipped = true;
    cig_clip_rect_t_stack_t *clip_rects = &current->buffers._peek(&current->buffers, 0)->clip_rects;
    cig_r clip_rect = cig_r_union(frame->absolute_rect, clip_rects->peek(clip_rects, 0));
    clip_rects->push(clip_rects, clip_rect);

    if (set_clip) {
      set_clip(cig_buffer(), clip_rect, clip_rects->size == 1);
    }
  }
}

static void pop_clip() {
  cig_clip_rect_t_stack_t *clip_rects = &current->buffers._peek(&current->buffers, 0)->clip_rects;
  CIG_UNUSED(clip_rects->_pop(clip_rects));
  cig_r restored_clip_rect = clip_rects->peek(clip_rects, 0);

  if (set_clip) {
    set_clip(cig_buffer(), restored_clip_rect, clip_rects->size == 1);
  }
}

#ifdef DEBUG

/*  ┌────────────┐
    │ DEBUG MODE │
    └────────────┘ */

void cig_set_layout_breakpoint_callback(cig_layout_breakpoint_callback_t fp) {
  layout_breakpoint_callback = fp;
}

void cig_enable_debug_stepper() {
  requested_layout_step_mode = true;
}

void cig_disable_debug_stepper() {
  current->step_mode = false;
}

void cig_trigger_layout_breakpoint(cig_r container, cig_r rect) {
  if (current->step_mode && layout_breakpoint_callback) {
    layout_breakpoint_callback(container, rect);
  }
}

#endif