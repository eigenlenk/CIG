#include "cigcore.h"
#include "cigcorem.h"
#include <string.h>
#include <assert.h>

cig__macro_ctx_st cig__macro_ctx = { 0 };

static cig_context *current = NULL;
static cig_set_clip_callback set_clip = NULL;

#ifdef DEBUG
static cig_layout_breakpoint_callback_t layout_breakpoint_callback = NULL;
static bool requested_layout_step_mode = false;
#endif

/*  Forward delcarations */
static CIG_OPTIONAL(cig_state*) find_state(cig_id);
static CIG_OPTIONAL(cig_scroll_state_t*) find_scroll_state(cig_id);
static CIG_OPTIONAL(cig_state *) enable_state();
static void handle_frame_hover(cig_frame*);
static void push_clip(cig_frame*);
static void pop_clip();
static cig_r calculate_rect_in_parent(cig_r, const cig_frame*);
static cig_r align_rect_in_parent(cig_r, cig_r, const cig_params*);
static bool next_layout_rect(cig_r, cig_frame*, cig_r*);
static cig_frame* push_frame(cig_r, cig_i, cig_params, bool (*)(cig_r, cig_r, cig_params*, cig_r*));
static void move_to_next_row(cig_params*);
static void move_to_next_column(cig_params*);
static double get_attribute_value_of_relative_to(cig_pin_attribute, double, cig_frame*, cig_frame*);

CIG_INLINED int limit(int v, const int minv_or_zero, const int maxv_or_zero) {
  if (maxv_or_zero > 0) { v = CIG_MIN(maxv_or_zero, v); }
  if (minv_or_zero > 0) { v = CIG_MAX(minv_or_zero, v); }
  return v;
}

/*  ┌─────────────┐
    │ CORE LAYOUT │
    └─────────────┘ */

void cig_init_context(cig_context *context) {
  register int i;

  context->frame_stack = INIT_STACK(cig_frame_ref);
  context->buffers = INIT_STACK(cig_buffer_element_t);
  context->input_state = (cig_input_state_t) { 0 };
  context->next_id = 0;
  context->tick = 1;
  context->delta_time = 0.f;
  context->elapsed_time = 0.f;
  context->frames.high = 0;

  for (i = 0; i < CIG_STATES_MAX; ++i) {
    context->state_list[i].id = 0;
    context->state_list[i].last_tick = context->tick;
  }
  
  for (i = 0; i < CIG_SCROLLABLE_ELEMENTS_MAX; ++i) {
    context->scroll_elements[i].id = 0;
    context->scroll_elements[i].last_tick = context->tick;
  }
}

void cig_begin_layout(
  cig_context *context,
  const cig_buffer_ref buffer,
  const cig_r rect,
  const float delta_time
) {
  current = context;

  current->frame_stack.clear(&current->frame_stack);
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

  current->frames.elements[0] = (cig_frame) {
    .id = current->next_id ? current->next_id : cig_hash("root"),
    .rect = cig_r_make(0, 0, rect.w, rect.h),
    .clipped_rect = rect,
    .absolute_rect = rect,
    .insets = cig_i_zero(),
    ._layout_function = NULL,
    ._parent = NULL,
    ._layout_params = (cig_params) { 0 },
    ._last_tick = current->tick,
    ._flags = OPEN
  };
  current->frame_stack.push(&current->frame_stack, &current->frames.elements[0]);
  current->frames.high = CIG_MAX(current->frames.high, 1);

  cig_push_buffer(buffer);
  current->next_id = 0;

#ifdef DEBUG
  cig_trigger_layout_breakpoint(cig_r_zero(), cig_r_make(0, 0, rect.w, rect.h));
#endif
}

void cig_end_layout() {
  register int i, j;
  for (i = 0; i < CIG_STATES_MAX; ++i) {
    if (current->state_list[i].last_tick != current->tick) {
      current->state_list[i].value.active = false;
    }
  }

  for (i = 1, j = 1; i < current->frames.high; ++i) {
    if (current->frames.elements[i]._last_tick == current->tick) {
      current->frames.elements[j++] = current->frames.elements[i];
    }
  }

  current->frames.high = j;

  ++current->tick;
}

cig_r cig_layout_rect() {
  return current->frames.elements[0].absolute_rect;
}

cig_buffer_ref cig_buffer() {
  return current->buffers.peek_ref(&current->buffers, 0)->buffer;
}

cig_frame* cig_push_frame_args(cig_args args) {
  return push_frame(args.rect, args.insets, args.params, args.builder);
}

cig_frame* cig_push_frame(const cig_r rect) {
  return push_frame(rect, current->default_insets, (cig_params){ 0 }, NULL);
}

cig_frame* cig_push_frame_insets(const cig_r rect, const cig_i insets) {
  return push_frame(rect, insets, (cig_params){ 0 }, NULL);
}

cig_frame* cig_push_frame_insets_params(const cig_r rect, const cig_i insets, const cig_params params) {
  return push_frame(rect, insets, params, NULL);
}

cig_frame* cig_push_layout_function(
  bool (*layout_function)(const cig_r, const cig_r, cig_params *, cig_r *),
  const cig_r rect,
  const cig_i insets,
  cig_params params
) {
  return push_frame(rect, insets, params, layout_function);
}

cig_frame* cig_pop_frame() {
  cig_frame *popped_frame = stack_cig_frame_ref_pop(cig_frame_stack());
  popped_frame->_flags &= ~OPEN;
  if (popped_frame->_flags & CLIPPED) {
    pop_clip();
  }
  cig__macro_ctx.last_closed = popped_frame;
  return popped_frame;
}

void cig_set_default_insets(cig_i insets) {
  current->default_insets = insets;
}

cig_frame* cig_current() {
  return stack_cig_frame_ref_peek(cig_frame_stack(), 0);
}

cig_r cig_convert_relative_rect(const cig_r rect) {
  const cig_buffer_element_t *buffer_element = current->buffers.peek_ref(&current->buffers, 0);
  const cig_frame *frame = cig_current();
  
  return cig_r_offset(
    rect,
    frame->absolute_rect.x + frame->insets.left - buffer_element->origin.x,
    frame->absolute_rect.y + frame->insets.top - buffer_element->origin.y
  );
}

cig_frame_ref_stack_t* cig_frame_stack() {
  return &current->frame_stack;
}

/*  ┌───────┐
    │ STATE │
    └───────┘ */

CIG_OPTIONAL(void *) cig_arena_allocate(cig_arena *arena, size_t bytes) {
  cig_arena *_arena = arena;

  if (!_arena) {
    cig_state *state = enable_state();
    if (!state) {
      return NULL;
    }
    _arena = &state->arena;
  }

  if (_arena->mapped + bytes >= CIG_STATE_MEM_ARENA_BYTES) {
    return NULL;
  }

  void *result = &_arena->bytes[_arena->mapped];
  _arena->mapped += bytes;
  return result;
}

CIG_OPTIONAL(void *) cig_arena_read(cig_arena *arena, bool from_start, size_t bytes) {
  cig_arena *_arena = arena;

  if (!_arena) {
    cig_state *state = enable_state();
    if (!state) {
      return NULL;
    }
    _arena = &state->arena;
  }

  if (_arena->read + bytes >= CIG_STATE_MEM_ARENA_BYTES) {
    return NULL;
  }

  if (from_start) {
    _arena->read = 0;
  }

  void *result = &_arena->bytes[_arena->read];
  _arena->read += bytes;
  return result;
}

/*  ┌──────────────────────────────┐
    │ TEMPORARY BUFFERS (ADVANCED) │
    └──────────────────────────────┘ */

void cig_push_buffer(const cig_buffer_ref buffer) {
  current->buffers.push(&current->buffers, (cig_buffer_element_t) {
    .buffer = buffer,
    .absolute_rect = cig_current()->absolute_rect,
    .origin = current->buffers.size == 0
      ? cig_v_zero()
      : cig_v_make(cig_current()->absolute_rect.x, cig_current()->absolute_rect.y),
     .clip_rects = INIT_STACK(cig_clip_rect_t)
  });
}

void cig_pop_buffer() {
  CIG_UNUSED(current->buffers.pop_ref(&current->buffers));
}

/*  ┌───────────────────────────┐
    │ MOUSE INTERACTION & FOCUS │
    └───────────────────────────┘ */

void cig_set_input_state(
  const cig_v position,
  cig_input_action_type action_mask
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

  const cig_input_action_type previous_action_mask = current->input_state.action_mask;

  current->input_state.position = position;
  current->input_state.action_mask = action_mask;

  current->input_state.click_state = (!current->input_state.action_mask && previous_action_mask)
    ? (current->elapsed_time - current->input_state._press_start_time) <= CIG_CLICK_EXPIRE_IN_SECONDS
      ? ENDED
      : EXPIRED
    : (action_mask & CIG_INPUT_PRIMARY_ACTION && !(previous_action_mask & CIG_INPUT_PRIMARY_ACTION)) || (action_mask & CIG_INPUT_SECONDARY_ACTION && !(previous_action_mask & CIG_INPUT_SECONDARY_ACTION))
      ? current->input_state.click_state == BEGAN
        ? NEITHER
        : BEGAN
      : NEITHER;

  if (current->input_state.action_mask & CIG_INPUT_PRIMARY_ACTION && !(previous_action_mask & CIG_INPUT_PRIMARY_ACTION)) {
    current->input_state.last_action_began = CIG_INPUT_PRIMARY_ACTION;
  } else if (current->input_state.action_mask & CIG_INPUT_SECONDARY_ACTION && !(previous_action_mask & CIG_INPUT_SECONDARY_ACTION)) {
    current->input_state.last_action_began = CIG_INPUT_SECONDARY_ACTION;
  } else {
    current->input_state.last_action_began = 0;
  }

  if (previous_action_mask & CIG_INPUT_PRIMARY_ACTION && !(current->input_state.action_mask & CIG_INPUT_PRIMARY_ACTION)) {
    current->input_state.last_action_ended = CIG_INPUT_PRIMARY_ACTION;
  } else if (previous_action_mask & CIG_INPUT_SECONDARY_ACTION && !(current->input_state.action_mask & CIG_INPUT_SECONDARY_ACTION)) {
    current->input_state.last_action_ended = CIG_INPUT_SECONDARY_ACTION;
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
  cig_frame *frame = cig_current();
  if (frame->_flags & INTERACTIBLE) {
    return;
  }
  frame->_flags |= INTERACTIBLE;
  if (frame->_flags & SUBTREE_INCLUSIVE_HOVER) {
    current->input_state._target_this_tick = frame->id;
  }
}

bool cig_hovered() {
  /*  See `handle_frame_hover` */
  return current->input_state.locked == false && (cig_current()->_flags & INTERACTIBLE) && cig_current()->id == current->input_state._target_prev_tick;
}

cig_input_action_type cig_pressed(
  const cig_input_action_type actions,
  const cig_press_flags options
) {
  if (!cig_hovered()) {
    return 0;
  }

  const cig_input_action_type action_mask = actions & current->input_state.action_mask;

  if (action_mask && ((options & CIG_PRESS_INSIDE) == false || (options & CIG_PRESS_INSIDE && current->input_state._press_target_id == cig_current()->id))) {
    return action_mask;
  } else {
    return 0;
  }
}

cig_input_action_type cig_clicked(
  const cig_input_action_type actions,
  const cig_click_flags options
) {
  if (!cig_hovered()) {
    return 0;
  }

  cig_input_action_type result;

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
      if (options & CIG_CLICK_STARTS_INSIDE && current->input_state._press_target_id != cig_current()->id) {
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
  cig_frame *frame = cig_current();
  frame->_flags |= FOCUSABLE;
  return cig_focused();
}

bool cig_focused() {
  return cig_focused_id() == cig_current()->id;
}

cig_id cig_focused_id() {
  return current->input_state._focus_target;
}

void cig_set_focused_id(cig_id id) {
  current->input_state._focus_target = id;
}

/*  ┌───────────┐
    │ SCROLLING │
    └───────────┘ */

bool cig_enable_scroll(cig_scroll_state_t *state) {
  cig_frame *frame = cig_current();
  frame->_scroll_state = state ? state : find_scroll_state(frame->id);
  cig_enable_clipping();
  return frame->_scroll_state != NULL;
}

cig_scroll_state_t* cig_scroll_state() {
  return cig_current()->_scroll_state;
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

/*  ┌────────────────┐
    │ LAYOUT HELPERS │
    └────────────────┘ */

cig_r cig_build_rect(size_t n, cig_pin refs[]) {
  register size_t i;
  int32_t x0, y0, x1, y1, w, h, cx, cy;
  double a = 1;
  uint32_t attrs = 0;
  cig_pin pin;

  cig_frame *cur = cig_current();

  for (i = 0; i < n; ++i) {
    pin = refs[i];

    cig_pin_attribute attr = pin.attribute == UNSPECIFIED
      ? pin.relation_attribute
      : pin.attribute;

    cig_pin_attribute rel_attr = pin.relation_attribute == UNSPECIFIED
      ? pin.attribute
      : pin.relation_attribute;

    assert(rel_attr);

    if (pin.relation) {
      assert(pin.relation->_flags & OPEN || pin.relation->_flags & RETAINED);
    }

    double v = get_attribute_value_of_relative_to(rel_attr, pin.value, pin.relation, cur);
    attrs |= CIG_BIT(attr);

    switch (attr) {
    case LEFT: x0 = v; break;
    case RIGHT: x1 = v; break;
    case TOP: y0 = v; break;
    case BOTTOM: y1 = v; break;
    case WIDTH: w = v; break;
    case HEIGHT: h = v; break;
    case CENTER_X: cx = v; break;
    case CENTER_Y: cy = v; break;
    case ASPECT: a = v; break;

    case LEFT_INSET:
    case RIGHT_INSET:
    case TOP_INSET:
    case BOTTOM_INSET:
      /* These don't make sense here, they can only be used to reference an element */
      break;

    default:
      break;
    }
  }

  /* Calculate missing values based on what we have */

  if (!(attrs & CIG_BIT(WIDTH)) && CIG_BIT(ASPECT)) {
    if (attrs & CIG_BIT(HEIGHT)) {
      w = round(h * a);
      attrs |= CIG_BIT(WIDTH);
    } else if (attrs & CIG_BIT(TOP) && attrs & CIG_BIT(BOTTOM)) {
      w = round((y1 - y0) * a);
      attrs |= CIG_BIT(WIDTH);
    }
  }

  if (!(attrs & CIG_BIT(HEIGHT)) && CIG_BIT(ASPECT)) {
    if (attrs & CIG_BIT(WIDTH)) {
      h = round(w / a);
      attrs |= CIG_BIT(HEIGHT);
    } else if (attrs & CIG_BIT(LEFT) && attrs & CIG_BIT(RIGHT)) {
      h = round((x1 - x0) / a);
      attrs |= CIG_BIT(HEIGHT);
    }
  }

  if (!(attrs & CIG_BIT(LEFT))) {
    if (attrs & CIG_BIT(RIGHT) && attrs & CIG_BIT(WIDTH)) {
      x0 = x1 - w;
    } else if ((attrs & CIG_BIT(CENTER_X)) && (attrs & CIG_BIT(WIDTH))) {
      x0 = cx - (w * 0.5);
    } else if ((attrs & CIG_BIT(CENTER_X)) && (attrs & CIG_BIT(RIGHT))) {
      x0 = cx - (x1 - cx);
    } else if (attrs & CIG_BIT(WIDTH)) {
      x0 = 0;
    } else {
      assert(false);
    }
    attrs |= CIG_BIT(LEFT);
  } 

  if (!(attrs & CIG_BIT(RIGHT))) {
    if (attrs & CIG_BIT(LEFT) && attrs & CIG_BIT(WIDTH)) {
      x1 = x0 + w;
    } else if ((attrs & CIG_BIT(CENTER_X)) && (attrs & CIG_BIT(WIDTH))) {
      x1 = cx + (w * 0.5);
    } else if ((attrs & CIG_BIT(CENTER_X)) && (attrs & CIG_BIT(LEFT))) {
      x1 = cx + (cx - x0);
    } else {
      assert(false);
    }
    attrs |= CIG_BIT(RIGHT);
  }

  if (!(attrs & CIG_BIT(TOP))) {
    if (attrs & CIG_BIT(BOTTOM) && attrs & CIG_BIT(HEIGHT)) {
      y0 = y1 - h;
    } else if ((attrs & CIG_BIT(CENTER_Y)) && (attrs & CIG_BIT(HEIGHT))) {
      y0 = cy - (h * 0.5);
    } else if ((attrs & CIG_BIT(CENTER_Y)) && (attrs & CIG_BIT(BOTTOM))) {
      y0 = cy - (y1 - cy);
    } else if (attrs & CIG_BIT(HEIGHT)) {
      y0 = 0;
    } else {
      assert(false);
    }
    attrs |= CIG_BIT(TOP);
  } 

  if (!(attrs & CIG_BIT(BOTTOM))) {
    if (attrs & CIG_BIT(TOP) && attrs & CIG_BIT(HEIGHT)) {
      y1 = y0 + h;
    } else if ((attrs & CIG_BIT(CENTER_Y)) && (attrs & CIG_BIT(HEIGHT))) {
      y1 = cy + (h * 0.5);
    } else if ((attrs & CIG_BIT(CENTER_Y)) && (attrs & CIG_BIT(TOP))) {
      y1 = cy + (cy - y0);
    } else {
      assert(false);
    }
    attrs |= CIG_BIT(BOTTOM);
  }

  return cig_r_make(x0, y0, x1 - x0, y1 - y0);
}

void cig_disable_culling() {
  cig_current()->_layout_params.flags |= CIG_LAYOUT_DISABLE_CULLING;
}

void cig_enable_clipping() {
  push_clip(cig_current());
}

void cig_set_next_id(cig_id id) {
  current->next_id = id;
}

unsigned int cig_depth() {
  return cig_frame_stack()->size;
}

cig_id cig_hash(const char *str) {
  /*  http://www.cse.yorku.ca/~oz/hash.html */
  register cig_id hash = 5381;
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

bool cig_default_layout_builder(
  const cig_r container, /* Rect into which sub-frames are laid out */
  const cig_r rect,      /* Proposed rect, generally from CIG_FILL */
  cig_params *prm,
  cig_r *result
) {
  const bool h_axis = prm->axis & CIG_LAYOUT_AXIS_HORIZONTAL;
  const bool v_axis = prm->axis & CIG_LAYOUT_AXIS_VERTICAL;
  const bool is_grid = h_axis && v_axis;

  int x = prm->_h_pos,
      y = prm->_v_pos,
      w,
      h;

  if (h_axis) {
    if (CIG_IS_AUTO(rect.w)) {
      if (prm->width > 0) {
        w = CIG_ANY_VALUE(rect.w, prm->width);
      } else if (prm->columns) {
        w = CIG_ANY_VALUE(rect.w, (container.w - ((prm->columns - 1) * prm->spacing)) / prm->columns);
      } else if (is_grid && prm->_h_size && prm->direction == CIG_LAYOUT_DIRECTION_VERTICAL) {
        w = CIG_ANY_VALUE(rect.w, prm->_h_size);
      } else {
        w = CIG_ANY_VALUE(rect.w, container.w - prm->_h_pos);
      }
    } else {
      w = CIG_IS_REL(rect.w) ? CIG_REL_VALUE(rect.w, container.w - prm->_h_pos) : rect.w;
    }
  } else {
    w = CIG_ANY_VALUE(rect.w, container.w - prm->_h_pos);

    /*  Reset any remaining horizontal positioning in case we modify axis mid-layout */
    prm->_h_pos = 0;
    prm->_h_size = 0;
  }

  if (v_axis) {
    if (CIG_IS_AUTO(rect.h)) {
      if (prm->height > 0) {
        h = CIG_ANY_VALUE(rect.h, prm->height);
      } else if (prm->rows) {
        h = CIG_ANY_VALUE(rect.h, (container.h - ((prm->rows - 1) * prm->spacing)) / prm->rows);
      } else if (is_grid && prm->_v_size && prm->direction == CIG_LAYOUT_DIRECTION_HORIZONTAL) {
        h = CIG_ANY_VALUE(rect.h, prm->_v_size);
      } else {
        h = CIG_ANY_VALUE(rect.h, container.h - prm->_v_pos);
      }
    } else {
      h = CIG_IS_REL(rect.h) ? CIG_REL_VALUE(rect.h, container.h - prm->_v_pos) : rect.h;
    }
  } else {
    h = CIG_ANY_VALUE(rect.h, container.h - prm->_v_pos);

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

  *result = align_rect_in_parent(cig_r_make(x, y, w, h), container, prm);
  
  return true;
}

cig_frame* cig_push_hstack(cig_r rect, cig_i insets, cig_params params) {
  params.axis = CIG_LAYOUT_AXIS_HORIZONTAL;
  return cig_push_layout_function(&cig_default_layout_builder, rect, insets, params);
}

cig_frame* cig_push_vstack(cig_r rect, cig_i insets , cig_params params) {
  params.axis = CIG_LAYOUT_AXIS_VERTICAL;
  return cig_push_layout_function(&cig_default_layout_builder, rect, insets, params);
}

cig_frame* cig_push_grid(cig_r rect, cig_i insets, cig_params params) {
  params.axis = CIG_LAYOUT_AXIS_HORIZONTAL | CIG_LAYOUT_AXIS_VERTICAL;
  return cig_push_layout_function(&cig_default_layout_builder, rect, insets, params);
}

/*  ┌─────────┐
    │ UTILITY │
    └─────────┘ */

float cig_delta_time() { return current->delta_time; }

float cig_elapsed_time() { return current->elapsed_time; }


/*  ┌───────────────────┐
    │ BACKEND CALLBACKS │
    └───────────────────┘ */

void cig_assign_set_clip(cig_set_clip_callback fp) {
  set_clip = fp;
}

/*  ┌────────────────────┐
    │ INTERNAL FUNCTIONS │
    └────────────────────┘ */

CIG_INLINED cig_r calculate_rect_in_parent(const cig_r rect, const cig_frame *parent) {
  const cig_r content_rect = cig_r_inset(parent->rect, parent->insets);

  return align_rect_in_parent(cig_r_make(
    /*  When X or Y component have REL flag set, they are relative to W & H respectively.
        AUTO is not taken into consideration here */
    CIG_IS_REL(rect.x) ? CIG_REL_VALUE(rect.x, content_rect.w) : rect.x,
    CIG_IS_REL(rect.y) ? CIG_REL_VALUE(rect.y, content_rect.h) : rect.y,
    limit(
      CIG_ANY_VALUE(rect.w, content_rect.w),
      parent->_layout_params.size_min.width,
      parent->_layout_params.size_max.width
    ),
    limit(
      CIG_ANY_VALUE(rect.h, content_rect.h),
      parent->_layout_params.size_min.height,
      parent->_layout_params.size_max.height
    )
  ), content_rect, &parent->_layout_params);
}

CIG_INLINED cig_r align_rect_in_parent(cig_r rect, cig_r parent_rect, const cig_params *prm) {
  switch (prm->alignment.horizontal) {
  case CIG_LAYOUT_ALIGNS_CENTER: {
    rect.x = (parent_rect.w - rect.w) * 0.5;
  } break;
  case CIG_LAYOUT_ALIGNS_RIGHT: {
    rect.x = (parent_rect.w - (rect.x+rect.w));
  } break;
  default: break;
  }

  switch (prm->alignment.vertical) {
  case CIG_LAYOUT_ALIGNS_CENTER: {
    rect.y = (parent_rect.h - rect.h) * 0.5;
  } break;
  case CIG_LAYOUT_ALIGNS_BOTTOM: {
    rect.y = (parent_rect.h - (rect.y+rect.h));
  } break;
  default: break;
  }

  return rect;
}

CIG_INLINED bool next_layout_rect(const cig_r proposed, cig_frame *parent, cig_r *result) {
  if (parent->_layout_function) {
    return (*parent->_layout_function)(
      cig_r_inset(parent->rect, parent->insets),
      proposed,
      &parent->_layout_params,
      result
    );
  } else {
    *result = calculate_rect_in_parent(proposed, parent);
    return true;
  }
}

static cig_frame* push_frame(
  const cig_r rect,
  const cig_i insets,
  cig_params params,
  bool (*layout_function)(cig_r, cig_r, cig_params*, cig_r*)
) {
  register size_t i;
  register cig_frame *f;

  cig_buffer_element_t *current_buffer = current->buffers.peek_ref(&current->buffers, 0);
  cig_frame *top = cig_current();

  if (top->_layout_params.limit.total > 0 && top->_layout_params._count.total == top->_layout_params.limit.total) {
    goto failure;
  }

  cig_r next;
  if (!next_layout_rect(rect, top, &next)) {
    top->_id_counter ++;
    goto failure;
  }

  top->content_rect = cig_r_containing(top->content_rect, next);

  if (top->_scroll_state) {
    next = cig_r_offset(next, -top->_scroll_state->offset.x, -top->_scroll_state->offset.y);
  }

  if (!(top->_layout_params.flags & CIG_LAYOUT_DISABLE_CULLING)
    && !cig_r_intersects(top->rect, cig_r_offset(next, top->rect.x+top->insets.left, top->rect.y+top->insets.top))) {
    top->_id_counter ++;
    goto failure;
  }

  const cig_id next_id = current->next_id
    ? current->next_id
    : (top->id + CIG_TINYHASH((top->id+top->_id_counter++), cig_depth()));

  cig_frame *new_frame = NULL;
  cig_frame_visibility previous_visibility = 0;

  /* 1. Try find a retained frame */
  for (i = 1; i < current->frames.high; ++i) {
    f = &current->frames.elements[i];

    if (f->_flags & RETAINED && f->id == next_id) {
      new_frame = f;

      if (new_frame->_last_tick != current->tick - 1) {
        previous_visibility = new_frame->visibility = 0;
      } else {
        previous_visibility = new_frame->visibility;
      }

      goto insert_frame;
    }
  }

  /* 2. Try find an available frame */
  for (i = current->frame_stack.size; i < current->frames.high; ++i) {
    f = &current->frames.elements[i];
    
    if (!(f->_flags & RETAINED) && !(f->_flags & OPEN)) {
      new_frame = f;
      goto insert_frame;
    }
  }

  /* 3. */
  if (current->frames.high < CIG_ELEMENTS_MAX) {
    new_frame = &current->frames.elements[current->frames.high++];
    goto insert_frame;
  }

  if (!new_frame) {
    goto failure;
  }

  insert_frame:

  top->_layout_params._count.total ++;

  cig_r absolute_rect = cig_convert_relative_rect(next);
  cig_r current_clip_rect = !current_buffer->clip_rects.size
    ? current_buffer->absolute_rect
    : current_buffer->clip_rects.peek(&current_buffer->clip_rects, 0);

  *new_frame = (cig_frame) {
    .id = next_id,
    .rect = next,
    .clipped_rect = cig_r_offset(cig_r_union(absolute_rect, current_clip_rect), -absolute_rect.x + next.x, -absolute_rect.y + next.y),
    .absolute_rect = absolute_rect,
    .insets = insets,
    .visibility = CIG_MIN(CIG_FRAME_VISIBLE, previous_visibility + 1),
    ._layout_function = layout_function,
    ._layout_params = params,
    ._parent = top,
    ._last_tick = current->tick,
    ._flags = OPEN
  };

  current->frame_stack.push(&current->frame_stack, new_frame);
  current->next_id = 0;

  if (cig__macro_ctx.open) { *cig__macro_ctx.open = new_frame; }
  if (cig__macro_ctx.retain) { CIG_UNUSED(cig_retain(new_frame)); }
  cig__macro_ctx.open = NULL;
  cig__macro_ctx.retain = 0;
  cig__macro_ctx.last_closed = NULL;

  handle_frame_hover(new_frame);

#ifdef DEBUG
  cig_trigger_layout_breakpoint(top->absolute_rect, absolute_rect);
#endif

  return new_frame;

  failure:
  if (cig__macro_ctx.open) { *cig__macro_ctx.open = NULL; }
  cig__macro_ctx.open = NULL;
  cig__macro_ctx.retain = 0;
  cig__macro_ctx.last_closed = NULL;
  return NULL;
}

CIG_INLINED void handle_frame_hover(cig_frame *frame) {
  if (cig_r_contains(frame->absolute_rect, current->input_state.position)) {
    frame->_flags |= HOVER;
    frame->_flags |= SUBTREE_INCLUSIVE_HOVER;
    cig_frame *parent = frame->_parent;
    while (parent) {
      parent->_flags |= SUBTREE_INCLUSIVE_HOVER;
      if (parent->_flags & FOCUSABLE) {
        if (current->input_state.click_state == BEGAN) {
          current->input_state._focus_target_this = parent->id;
        }
      }
      parent = parent->_parent;
    }
  }
}

static CIG_OPTIONAL(cig_state*) find_state(const cig_id id) {
  register int i, open = -1, stale = -1;

  /*  Find a state with a matching ID, with no ID yet, or a stale state */
  for (i = 0; i < CIG_STATES_MAX; ++i) {
    if (current->state_list[i].id == id) {
      current->state_list[i].value.active = true;
      current->state_list[i].last_tick = current->tick;
      return &current->state_list[i].value;
    }
    else if (open < 0 && !current->state_list[i].id) {
      open = i;
    }
    else if (stale < 0 && !current->state_list[i].value.active) {
      stale = i;
    }
  }

  const int result = open >= 0 ? open : stale;

  if (result >= 0) {
    current->state_list[result].id = id;
    current->state_list[result].last_tick = current->tick;
    memset(&current->state_list[result].value.arena.bytes, 0, CIG_STATE_MEM_ARENA_BYTES);
    current->state_list[result].value.arena.mapped = 0;
    current->state_list[result].value.active = true;
    return &current->state_list[result].value;
  }

  return NULL;
}

static CIG_OPTIONAL(cig_scroll_state_t*) find_scroll_state(const cig_id id) {
  register int i, first_unused = -1, first_stale = -1;

  for (i = 0; i < CIG_SCROLLABLE_ELEMENTS_MAX; ++i) {
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
    current->scroll_elements[i].last_tick = current->tick;

    return &current->scroll_elements[i].value;
  }
}

CIG_INLINED CIG_OPTIONAL(cig_state *) enable_state() {
  cig_frame *frame = cig_current();
  if (frame->_state) {
    return frame->_state;
  }
  if ((frame->_state = find_state(frame->id))) {
    frame->_state->arena.mapped = 0;
    frame->_state->arena.read = 0;
  }
  /*if (frame->_state) {
    frame->_state->_flags |= RETAINED;
  }*/
  return frame->_state;
}

static void push_clip(cig_frame *frame) {
  if (!(frame->_flags & CLIPPED)) {
    cig_buffer_element_t *buffer_element = current->buffers.peek_ref(&current->buffers, 0);
    cig_clip_rect_t_stack_t *clip_rects = &buffer_element->clip_rects;
    /* Clip against current clip rect, or just use the absolute frame of current buffer */
    cig_r clip_rect = cig_r_union(frame->absolute_rect, !clip_rects->size
      ? buffer_element->absolute_rect
      : clip_rects->peek(clip_rects, 0)
    );
    clip_rects->push(clip_rects, clip_rect);

    if (set_clip) {
      set_clip(cig_buffer(), clip_rect, false);
    }

    frame->_flags |= CLIPPED;
  }
}

static void pop_clip() {
  cig_buffer_element_t *buf_element = current->buffers.peek_ref(&current->buffers, 0);
  cig_clip_rect_t_stack_t *clip_rects = &buf_element->clip_rects;
  CIG_UNUSED(clip_rects->pop_ref(clip_rects));

  if (set_clip) {
    if (!clip_rects->size) {
      set_clip(cig_buffer(), buf_element->absolute_rect, true);
    } else {
      set_clip(cig_buffer(), clip_rects->peek(clip_rects, 0), false);
    }
  }
}

CIG_INLINED void move_to_next_row(cig_params *prm) {
  prm->_h_pos = 0;
  prm->_v_pos += (prm->_v_size + prm->spacing);
  prm->_h_size = 0;
  prm->_v_size = 0;
  prm->_count.h_cur = 0;
}

CIG_INLINED void move_to_next_column(cig_params *prm) {
  prm->_v_pos = 0;
  prm->_h_pos += (prm->_h_size + prm->spacing);
  prm->_h_size = 0;
  prm->_v_size = 0;
  prm->_count.v_cur = 0;
}

CIG_INLINED double get_attribute_value_of_relative_to(
  cig_pin_attribute attribute,
  double _value,
  cig_frame *of_frame, 
  cig_frame *relative_to_frame
) {
  int32_t value = _value;
  switch (attribute) {
  case LEFT:
    assert(of_frame);
    return (CIG_IS_REL(value) ? CIG_REL_VALUE(value, of_frame->absolute_rect.w) : value)
      + of_frame->absolute_rect.x - relative_to_frame->absolute_rect.x;

  case RIGHT:
    assert(of_frame);
    return (CIG_IS_REL(value) ? CIG_REL_VALUE(value, of_frame->absolute_rect.w) : value)
      + of_frame->absolute_rect.w + of_frame->absolute_rect.x - relative_to_frame->absolute_rect.x;

  case TOP:
    assert(of_frame);
    return (CIG_IS_REL(value) ? CIG_REL_VALUE(value, of_frame->absolute_rect.h) : value)
      + of_frame->absolute_rect.y - relative_to_frame->absolute_rect.y;

  case BOTTOM:
    assert(of_frame);
    return (CIG_IS_REL(value) ? CIG_REL_VALUE(value, of_frame->absolute_rect.h) : value)
      + of_frame->absolute_rect.h + of_frame->absolute_rect.y - relative_to_frame->absolute_rect.y;

  case LEFT_INSET:
    {
      assert(of_frame);
      const cig_r r0 = cig_r_inset(of_frame->absolute_rect, of_frame->insets);
      const cig_r r1 = cig_r_inset(relative_to_frame->absolute_rect, relative_to_frame->insets);
      return (CIG_IS_REL(value) ? CIG_REL_VALUE(value, r0.w) : value) + r0.x - r1.x;
    }

  case RIGHT_INSET:
    {
      assert(of_frame);
      const cig_r r0 = cig_r_inset(of_frame->absolute_rect, of_frame->insets);
      const cig_r r1 = cig_r_inset(relative_to_frame->absolute_rect, relative_to_frame->insets);
      return (CIG_IS_REL(value) ? CIG_REL_VALUE(value, r0.w) : value) + r0.w + r0.x - r1.x;
    }

  case TOP_INSET:
    {
      assert(of_frame);
      const cig_r r0 = cig_r_inset(of_frame->absolute_rect, of_frame->insets);
      const cig_r r1 = cig_r_inset(relative_to_frame->absolute_rect, relative_to_frame->insets);
      return (CIG_IS_REL(value) ? CIG_REL_VALUE(value, r0.h) : value) + r0.y - r1.y;
    }

  case BOTTOM_INSET:
    {
      assert(of_frame);
      const cig_r r0 = cig_r_inset(of_frame->absolute_rect, of_frame->insets);
      const cig_r r1 = cig_r_inset(relative_to_frame->absolute_rect, relative_to_frame->insets);
      return (CIG_IS_REL(value) ? CIG_REL_VALUE(value, r0.h) : value) + r0.h + r0.y - r1.y;
    }

  case WIDTH:
    if (CIG_IS_REL(value)) {
      assert(of_frame);
      return CIG_REL_VALUE(value, of_frame->rect.w);
    } else {
      return value + (of_frame ? of_frame->rect.w : 0);
    }

  case HEIGHT:
    if (CIG_IS_REL(value)) {
      assert(of_frame);
      return CIG_REL_VALUE(value, of_frame->rect.h);
    } else {
      return value + (of_frame ? of_frame->rect.h : 0);
    }

  case CENTER_X:
    assert(of_frame);
    return (CIG_IS_REL(value) ? CIG_REL_VALUE(value, of_frame->absolute_rect.w) : value) + (of_frame->absolute_rect.w * 0.5) + of_frame->absolute_rect.x - relative_to_frame->absolute_rect.x;

  case CENTER_Y:
    assert(of_frame);
    return (CIG_IS_REL(value) ? CIG_REL_VALUE(value, of_frame->absolute_rect.h) : value) + (of_frame->absolute_rect.h * 0.5) + of_frame->absolute_rect.y - relative_to_frame->absolute_rect.y;

  case ASPECT:
    return of_frame ? ((double)of_frame->absolute_rect.w / of_frame->absolute_rect.h) : _value;

  default:
    break;
  }

  return _value;
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