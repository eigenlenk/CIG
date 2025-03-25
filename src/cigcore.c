#include "cigcore.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <math.h>

static cig_context_t *current = NULL;

/* Forward delcarations */
static CIG_OPTIONAL(cig_state_t*) find_state(cig_id_t);
static CIG_OPTIONAL(cig_scroll_state_t*) find_scroll_state(cig_id_t);
static void handle_frame_hover(const cig_frame_t*);
static void push_clip(cig_frame_t*);
static void pop_clip();
static cig_rect_t resolve_size(cig_rect_t, const cig_frame_t*);
static bool next_layout_rect(cig_rect_t, cig_frame_t*, cig_rect_t*);
static bool push_frame(cig_rect_t, cig_insets_t, cig_layout_params_t, bool (*)(cig_rect_t, cig_rect_t, cig_layout_params_t*, cig_rect_t*));
CIG_INLINED int tinyhash(int a, int b) { return (a * 31) ^ (b * 17); }

/* ┌───────────────┐
───┤  CORE LAYOUT  │
   └───────────────┘ */

void cig_init_context(cig_context_t *context) {
  context->frames = INIT_STACK(cig_frame_t);
  context->buffers = INIT_STACK(cig_buffer_element_t);
  context->input_state = (cig_input_state_t) { 0 };
  context->next_id = 0;
  context->tick = 1;
  
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
  const cig_rect_t rect
) {
  current = context;
  
  current->frames.clear(&current->frames);
  current->buffers.clear(&current->buffers);
  
  current->default_insets = cig_insets_zero();
  
  current->frames.push(&current->frames, (cig_frame_t) {
		0,
		.id = current->next_id ? current->next_id : cig_hash("root"),
		.rect = cig_rect_make(0, 0, rect.w, rect.h),
		.clipped_rect = rect,
		.absolute_rect = rect,
		.insets = cig_insets_zero(),
		._layout_function = NULL,
		._layout_params = (cig_layout_params_t){ 0 }
	});

	cig_push_buffer(buffer);
	cig_enable_clipping();
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
	
  
	/* Check for inactive states and filter them out by shifting active states back */
	/*for (register int i = current->state_list.size - 1; i >= 0; --i) {
		s = &current->state_list.values[i];
		if (s->last_tick != current->tick) {
			if (last) {
				*s = *last;
				last = NULL;
			} else {
				s->id = 0;
				s->activation_state = INACTIVE;
			}
			current->state_list.size --;
		} else if (!last) {
			last = s;
		}
	}*/
}

cig_buffer_ref cig_buffer() {
  return current->buffers._peek(&current->buffers, 0)->buffer;
}

bool cig_push_frame(const cig_rect_t rect) {
	return push_frame(rect, current->default_insets, (cig_layout_params_t){ 0 }, NULL);
}

bool cig_push_frame_insets(const cig_rect_t rect, const cig_insets_t insets) {
	return push_frame(rect, insets, (cig_layout_params_t){ 0 }, NULL);
}

bool cig_push_frame_insets_params(const cig_rect_t rect, const cig_insets_t insets, const cig_layout_params_t params) {
	return push_frame(rect, insets, params, NULL);
}

bool cig_push_layout_function(
	bool (*layout_function)(const cig_rect_t, const cig_rect_t, cig_layout_params_t *, cig_rect_t *),
	const cig_rect_t rect,
	const cig_insets_t insets,
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

void cig_set_default_insets(cig_insets_t insets) {
  current->default_insets = insets;
}

cig_frame_t* cig_frame() {
  return stack_cig_frame_t__peek(cig_frame_stack(), 0);
}

cig_rect_t cig_convert_relative_rect(const cig_rect_t rect) {
	const cig_buffer_element_t *buffer_element = current->buffers._peek(&current->buffers, 0);
	const cig_frame_t *frame = cig_frame();
	
	return cig_rect_offset(
		resolve_size(rect, frame),
		frame->absolute_rect.x + frame->insets.left - buffer_element->origin.x,
		frame->absolute_rect.y + frame->insets.top - buffer_element->origin.y
	);
}

cig_frame_t_stack_t* cig_frame_stack() {
	return &current->frames;
}

/* ┌─────────┐
───┤  STATE  │
   └─────────┘ */
   
CIG_OPTIONAL(cig_state_t*) cig_state() {
  return find_state(cig_frame()->id);
}

/* ┌────────────────────────────────┐
───┤  TEMPORARY BUFFERS (ADVANCED)  │
   └────────────────────────────────┘ */
	 
void cig_push_buffer(const cig_buffer_ref buffer) {
	cig_buffer_element_t buffer_element = {
		.buffer = buffer,
		.origin = current->buffers.size == 0
			? cig_vec2_zero()
			: cig_vec2_make(cig_frame()->absolute_rect.x, cig_frame()->absolute_rect.y),
     .clip_rects = INIT_STACK(cig_clip_rect_t)
	};
  
  buffer_element.clip_rects.push(&buffer_element.clip_rects, cig_frame()->absolute_rect);
  current->buffers.push(&current->buffers, buffer_element);
}

void cig_pop_buffer() {
  CIG_UNUSED(current->buffers._pop(&current->buffers));
}

/* ┌─────────────────────┐
───┤  MOUSE INTERACTION  │
   └─────────────────────┘ */
	 
void cig_set_input_state(
	const cig_vec2_t position,
	unsigned int action_mask
) {
	if (current->input_state.locked == false) {
		current->input_state._target_prev_tick = current->input_state._target_this_tick;
		current->input_state._target_this_tick = 0;
	}
	
	const cig_input_action_type_t previous_action_mask = current->input_state.action_mask;

	current->input_state.position = position;
	current->input_state.action_mask = action_mask;

	current->input_state.click_state = (!current->input_state.action_mask && previous_action_mask)
		? (current->tick - current->input_state._press_start_tick) < 12
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

	if (current->input_state.click_state == BEGAN) {
		current->input_state._press_start_tick = current->tick;
		current->input_state._press_target_id = current->input_state._target_prev_tick;
	}

	if (current->input_state.action_mask) {
		if (!current->input_state.drag.active) {
			current->input_state.drag.active = true;
			current->input_state.drag.start_position = current->input_state.position;
			current->input_state.drag.change = cig_vec2_zero();
		} else {
			current->input_state.drag.change = cig_vec2_sub(current->input_state.position, current->input_state.drag.start_position);
		}
	} else if (current->input_state.drag.active) {
		current->input_state.drag.active = false;
		current->input_state.drag.change = cig_vec2_zero();
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
	/* See `handle_frame_hover` */
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
	
	if (options & CIG_CLICK_ON_PRESS) {
		cig_input_action_type_t result;
		if (current->input_state.click_state == BEGAN && (result = actions & current->input_state.action_mask)) {
			return result;
		}
	} else {
		if (current->input_state.click_state == ENDED || (current->input_state.click_state == EXPIRED && !(options & CIG_CLICK_EXPIRE))) {
			if (options & CIG_CLICK_STARTS_INSIDE && current->input_state._press_target_id != cig_frame()->id) {
				return 0;
			}
			return actions & current->input_state.last_action_ended;
		}
	}
	
	return 0;
}

/* ┌─────────────┐
───┤  SCROLLING  │
   └─────────────┘ */

bool cig_enable_scroll(cig_scroll_state_t *state) {
  cig_frame_t *frame = cig_frame();
  frame->_scroll_state = state ? state : find_scroll_state(frame->id);
	cig_enable_clipping();
	return frame->_scroll_state != NULL;
}

cig_scroll_state_t* cig_scroll_state() {
	return cig_frame()->_scroll_state;
}

void cig_set_offset(cig_vec2_t offset) {
	assert(cig_scroll_state());
	cig_scroll_state()->offset = offset;
}

void cig_change_offset(cig_vec2_t delta) {
	assert(cig_scroll_state());
	cig_scroll_state()->offset = cig_vec2_add(cig_scroll_state()->offset, delta);
}

cig_vec2_t cig_offset() {
	assert(cig_scroll_state());
	return cig_scroll_state()->offset;
}

cig_vec2_t cig_content_size() {
	assert(cig_scroll_state());
	return cig_scroll_state()->content_size;
}

/* ┌──────────────────┐
───┤  LAYOUT HELPERS  │
   └──────────────────┘ */
	 
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
	/* http://www.cse.yorku.ca/~oz/hash.html */
	register cig_id_t hash = 5381;
	register int c;
	while ((c = *str++)) {
		hash = ((hash << 5) + hash) + c; /* === hash * 33 + c */
	}
	return hash;
}

void cig_empty() {
	cig_push_frame(CIG_FILL);
	cig_pop_frame();
}

void cig_spacer(const int size) {
	cig_push_frame(CIG_FILL_H(size));
	cig_pop_frame();
}

bool cig_default_layout_builder(
	const cig_rect_t container, /* Rect into which sub-frames are laid out */
	const cig_rect_t rect, /* Proposed rect, generally from CIG_FILL */
	cig_layout_params_t *prm,
	cig_rect_t *result
) {
	const bool h_axis = prm->axis & CIG_LAYOUT_AXIS_HORIZONTAL;
	const bool v_axis = prm->axis & CIG_LAYOUT_AXIS_VERTICAL;
	const bool is_grid = h_axis && v_axis;
	
	int x = prm->_h_pos,
	    y = prm->_v_pos,
			w,
			h;

	void move_to_next_row() {
		prm->_h_pos = 0;
		prm->_v_pos += (prm->_v_size + prm->spacing);
		prm->_h_size = CIG_FILL_CONSTANT;
		prm->_v_size = CIG_FILL_CONSTANT;
		prm->_count.h_cur = 0;
	}
	
	void move_to_next_column() {
		prm->_v_pos = 0;
		prm->_h_pos += (prm->_h_size + prm->spacing);
		prm->_h_size = CIG_FILL_CONSTANT;
		prm->_v_size = CIG_FILL_CONSTANT;
		prm->_count.v_cur = 0;
	}
	
	if (h_axis) {
		if (rect.w == CIG_FILL_CONSTANT) {
			if (prm->width > 0) {
				w = prm->width;
			} else if (prm->columns) {
				w = (container.w - ((prm->columns - 1) * prm->spacing)) / prm->columns;
			} else if (prm->_h_size && prm->direction == CIG_LAYOUT_DIRECTION_DOWN) {
				w = prm->_h_size;
			}	else {
				w = container.w - prm->_h_pos;
			}
		} else {
			w = rect.w;
		}
	} else {
		w = rect.w == CIG_FILL_CONSTANT
			? container.w - prm->_h_pos
			: rect.w;
		
		/* Reset any remaining horizontal positioning in case we modify axis mid-layout */
		prm->_h_pos = 0;
		prm->_h_size = CIG_FILL_CONSTANT;
	}
	
	if (v_axis) {
		if (rect.h == CIG_FILL_CONSTANT) {
			if (prm->height > 0) {
				h = prm->height;
			} else if (prm->rows) {
				h = (container.h - ((prm->rows - 1) * prm->spacing)) / prm->rows;
			} else if (prm->_v_size && prm->direction == CIG_LAYOUT_DIRECTION_LEFT) {
				h = prm->_v_size;
			}	else {
				h = container.h - prm->_v_pos;
			}
		} else {
			h = rect.h;
		}
	} else {
		h = rect.h == CIG_FILL_CONSTANT
			? container.h - prm->_v_pos
			: rect.h;
		
		/* Reset any remaining vertical positioning in case we modify axis mid-layout */
		prm->_v_pos = 0;
		prm->_v_size = CIG_FILL_CONSTANT;
	}
	
	if (h_axis && v_axis) {
		/* Can we fit the new frame onto current axis? */
		switch (prm->direction) {
			case CIG_LAYOUT_DIRECTION_LEFT: {
				if ((prm->limit.horizontal && prm->_count.h_cur == prm->limit.horizontal) || prm->_h_pos + w > container.w) {
					move_to_next_row();
					x = 0;
					y = prm->_v_pos;
				}
				
				prm->_h_pos += (w + prm->spacing);
				prm->_h_size = CIG_MAX(prm->_h_size, w);
				prm->_v_size = CIG_MAX(prm->_v_size, h);
				prm->_count.h_cur ++;
				
				if (prm->_h_pos >= container.w) {
					move_to_next_row();
				}
			} break;
			case CIG_LAYOUT_DIRECTION_DOWN: {
				if ((prm->limit.vertical && prm->_count.v_cur == prm->limit.vertical) || prm->_v_pos + h > container.h) {
					move_to_next_column();
					y = 0;
					x = prm->_h_pos;
				}
				
				prm->_v_pos += (h + prm->spacing);
				prm->_h_size = CIG_MAX(prm->_h_size, w);
				prm->_v_size = CIG_MAX(prm->_v_size, h);
				prm->_count.v_cur ++;
				
				if (prm->_v_pos >= container.h) {
					move_to_next_column();
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

	*result = cig_rect_make(x, y, w, h);
	
	return true;
}

/* ╔════════════════════════════════════════════╗
   ║            INTERNAL FUNCTIONS              ║
   ╚════════════════════════════════════════════╝ */

static cig_rect_t resolve_size(const cig_rect_t rect, const cig_frame_t *parent) {
  const cig_rect_t content_rect = cig_rect_inset(parent->rect, parent->insets);

	return cig_rect_make(
		rect.x,
		rect.y,
		rect.w == CIG_FILL_CONSTANT ? content_rect.w : rect.w,
		rect.h == CIG_FILL_CONSTANT ? content_rect.h : rect.h
	);
}

static bool next_layout_rect(const cig_rect_t proposed, cig_frame_t *parent, cig_rect_t *result) {
	if (parent->_layout_function) {
		return (*parent->_layout_function)(
      cig_rect_inset(parent->rect, parent->insets),
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
	const cig_rect_t rect,
	const cig_insets_t insets,
	cig_layout_params_t params,
	bool (*layout_function)(cig_rect_t, cig_rect_t, cig_layout_params_t*, cig_rect_t*)
) {
	cig_buffer_element_t *current_buffer = current->buffers._peek(&current->buffers, 0);
	cig_frame_t *top = cig_frame();
	
	if (top->_layout_params.limit.total > 0 && top->_layout_params._count.total == top->_layout_params.limit.total) {
		return false;
	}
	
	cig_rect_t next;
	if (!next_layout_rect(rect, top, &next)) {
		top->_id_counter ++;
		return false;
	}
	
  if (top->_scroll_state) {
		top->_scroll_state->content_size.x = CIG_MAX(top->_scroll_state->content_size.x, next.x + next.w);
		top->_scroll_state->content_size.y = CIG_MAX(top->_scroll_state->content_size.y, next.y + next.h);
	  next = cig_rect_offset(next, -top->_scroll_state->offset.x, -top->_scroll_state->offset.y);
  }

	if (!(top->_layout_params.flags & CIG_LAYOUT_DISABLE_CULLING)
		&& !cig_rect_intersects(top->rect, cig_rect_offset(next, top->rect.x+top->insets.left, top->rect.y+top->insets.top))) {
		top->_id_counter ++;
		return false;
	}

	top->_layout_params._count.total ++;

	cig_rect_t absolute_rect = cig_convert_relative_rect(next);
	cig_rect_t current_clip_rect = current_buffer->clip_rects.peek(&current_buffer->clip_rects, 0);

  current->frames.push(&current->frames, (cig_frame_t) {
		0,
		.id = current->next_id
      ? current->next_id
      : (top->id + tinyhash(top->id+top->_id_counter++, cig_depth())),
		.rect = next,
		.clipped_rect = cig_rect_offset(cig_rect_union(absolute_rect, current_clip_rect), -absolute_rect.x + next.x, -absolute_rect.y + next.y),
		.absolute_rect = absolute_rect,
    .insets = insets,
		._layout_function = layout_function,
		._layout_params = params,
    ._clipped = false,
    ._scroll_state = NULL
	});
  
	current->next_id = 0;

	return true;
}

static void handle_frame_hover(const cig_frame_t *frame) {
	if (cig_rect_contains(frame->absolute_rect, current->input_state.position)) {
		current->input_state._target_this_tick = frame->id;
	}
}

static CIG_OPTIONAL(cig_state_t*) find_state(const cig_id_t id) {
  register int open = -1, stale = -1;
  
  /* Find a state with a matching ID, with no ID yet, or a stale state */
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
    memset(&current->state_list[result].value.data, 0, CIG_STATE_MEM_ARENA_BYTES);
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
    /* Prefer an unused slot, use stale when nothing else is available */
    int i = (first_unused >= 0) ? first_unused : first_stale;
    
    current->scroll_elements[i].id = id;
    current->scroll_elements[i].value.offset = cig_vec2_zero();
    current->scroll_elements[i].value.content_size = cig_vec2_zero();
		current->scroll_elements[i].last_tick = current->tick;
		
    return &current->scroll_elements[i].value;
  }
}

static void push_clip(cig_frame_t *frame) {
  if (frame->_clipped == false) {
		frame->_clipped = true;
		cig_clip_rect_t_stack_t *clip_rects = &current->buffers._peek(&current->buffers, 0)->clip_rects;
    clip_rects->push(clip_rects, cig_rect_union(frame->absolute_rect, clip_rects->peek(clip_rects, 0)));
    
		// Check if graphics module is included
    // TODO: Set new clip region for the renderer
  }
}

static void pop_clip() {
  cig_clip_rect_t_stack_t *clip_rects = &current->buffers._peek(&current->buffers, 0)->clip_rects;
  
  CIG_UNUSED(clip_rects->_pop(clip_rects));
  
  // Check if graphics module is included
  // TODO: Set previous clip region for the renderer
}
