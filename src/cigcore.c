#include "cigcore.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <math.h>

/* Declare internal types */
typedef cig_frame_t cig_clip_frame_t;
#define STACK_CAPACITY_cig_clip_frame_t CIG_BUFFER_CLIP_REGIONS_MAX
DECLARE_ARRAY_STACK_T(cig_clip_frame_t);

typedef struct {
	cig_buffer_ref buffer;
	cig_vec2_t origin;
	stack_cig_clip_frame_t_t clip_frames;
} cig_buffer_element_t;

typedef struct {
  cig_id_t id;
	unsigned int last_tick;
  cig_scroll_state_t value;
} cig_scroll_state_element_t;

typedef struct {
	cig_id_t id;
	enum {
		INACTIVE = 0,
		ACTIVATED,
		ACTIVE
	} activation_state;
	unsigned int last_tick;
	unsigned char data[CIG_STATE_MEM_SIZE_BYTES];
} cig_state_t;

typedef cig_state_t* cig_state_ptr_t;
#define STACK_CAPACITY_cig_state_ptr_t CIG_STATES_MAX
DECLARE_ARRAY_STACK_T(cig_state_ptr_t);

#define STACK_CAPACITY_cig_buffer_element_t CIG_BUFFERS_MAX
DECLARE_ARRAY_STACK_T(cig_buffer_element_t);

/* Define internal state */
static stack_cig_state_ptr_t_t widget_states = INIT_STACK(cig_state_ptr_t);
static stack_cig_element_t_t elements = INIT_STACK(cig_element_t);
static stack_cig_buffer_element_t_t buffers = INIT_STACK(cig_buffer_element_t);
static struct {
	cig_state_t values[CIG_STATES_MAX];
	size_t size;
} _state_list = { 0 };
static cig_scroll_state_element_t _scroll_elements[CIG_SCROLLABLE_ELEMENTS_MAX];
static cig_input_state_t input_state = { 0 };
static cig_id_t next_id = 0;
static unsigned int tick = 0;

/* Forward delcarations */
// static cig_state_t* 				im_state(const cig_id_t, const imgui_widget_t);
static cig_scroll_state_t* find_scroll_state(cig_id_t);
static void handle_element_hover(const cig_element_t*);
static void push_clip(cig_element_t*);
static void pop_clip();
static cig_frame_t resolve_size(cig_frame_t, const cig_element_t*);
static bool next_layout_frame(cig_frame_t, cig_element_t *, cig_frame_t *);
static bool push_frame(cig_frame_t, cig_insets_t, cig_layout_params_t, bool (*)(cig_frame_t, cig_frame_t, cig_layout_params_t*, cig_frame_t*));
CIG_INLINED int tinyhash(int a, int b) { return (a * 31) ^ (b * 17); }

/* ┌───────────────┐
───┤  CORE LAYOUT  │
   └───────────────┘ */

void cig_begin_layout(const cig_buffer_ref buffer, const cig_frame_t frame) {
  widget_states.clear(&widget_states);
  elements.clear(&elements);
  buffers.clear(&buffers);
  
  elements.push(&elements, (cig_element_t) {
		0,
		.id = next_id ? next_id : cig_hash("root"),
		.frame = cig_frame_make(0, 0, frame.w, frame.h),
		.clipped_frame = frame,
		.absolute_frame = frame,
		.insets = cig_insets_zero(),
		._layout_function = NULL,
		._layout_params = (cig_layout_params_t){ 0, .flags = CIG_DEFAULT_LAYOUT_FLAGS }
	});

	cig_push_buffer(buffer);
	cig_enable_clipping();
	next_id = 0;
}

void cig_end_layout() {
	register cig_state_t *s, *last = NULL;
	
	++tick;
	
	/* Check for inactive states and filter them out by shifting active states back */
	for (register int i = _state_list.size - 1; i >= 0; --i) {
		s = &_state_list.values[i];
		if (s->last_tick != tick) {
			if (last) {
				*s = *last;
				last = NULL;
			} else {
				s->id = 0;
				s->activation_state = INACTIVE;
			}
			_state_list.size --;
		} else if (!last) {
			last = s;
		}
	}
}

void cig_reset_internal_state() {
	int i;
	for (i = 0; i < CIG_STATES_MAX; ++i) {
    _state_list.values[i].id = 0;
  }
  for (i = 0; i < CIG_SCROLLABLE_ELEMENTS_MAX; ++i) {
    _scroll_elements[i].id = 0;
  }
	_state_list.size = 0;
	input_state.locked = false;
	input_state._press_target_id = 0;
	input_state._target_prev_frame = 0;
	input_state._target_this_frame = 0;
	input_state._press_start_tick = 0;
	input_state.drag.active = false;
	input_state.drag.start_position = cig_vec2_zero();
	input_state.drag.change = cig_vec2_zero();
  tick = 0;
	next_id = 0;
}

cig_buffer_ref cig_buffer() {
  return buffers._peek(&buffers, 0)->buffer;
}

bool cig_push_frame(const cig_frame_t frame) {
	return push_frame(frame, cig_insets_zero(), (cig_layout_params_t){ 0, .flags = CIG_DEFAULT_LAYOUT_FLAGS }, NULL);
}

bool cig_push_frame_insets(
	const cig_frame_t frame,
	const cig_insets_t insets
) {
	return push_frame(frame, insets, (cig_layout_params_t){ 0, .flags = CIG_DEFAULT_LAYOUT_FLAGS }, NULL);
}

bool cig_push_frame_insets_params(
	const cig_frame_t frame,
	const cig_insets_t insets,
	const cig_layout_params_t params
) {
	return push_frame(frame, insets, params, NULL);
}

bool cig_push_layout_function(
	bool (*layout_function)(const cig_frame_t, const cig_frame_t, cig_layout_params_t *, cig_frame_t *),
	const cig_frame_t frame,
	const cig_insets_t insets,
	cig_layout_params_t params
) {
	return push_frame(frame, insets, params, layout_function);
}

cig_element_t* cig_pop_frame() {
  cig_element_t *popped_element = stack_cig_element_t__pop(cig_element_stack());

  if (popped_element->_clipped) {
    popped_element->_clipped = false;
    pop_clip();
  }
  
  return popped_element;
}

cig_element_t* cig_element() {
  return stack_cig_element_t__peek(cig_element_stack(), 0);
}

cig_frame_t cig_convert_relative_frame(const cig_frame_t frame) {
	const cig_buffer_element_t *buffer_element = buffers._peek(&buffers, 0);
	const cig_element_t *element = cig_element();
	
	return cig_frame_offset(
		resolve_size(frame, element),
		element->absolute_frame.x + element->insets.left - buffer_element->origin.x,
		element->absolute_frame.y + element->insets.top - buffer_element->origin.y
	);
}

stack_cig_element_t_t* cig_element_stack() {
	return &elements;
}

/* ┌────────────────────────────────┐
───┤  TEMPORARY BUFFERS (ADVANCED)  │
   └────────────────────────────────┘ */
	 
void cig_push_buffer(const cig_buffer_ref buffer) {
	cig_buffer_element_t buffer_element = {
		.buffer = buffer,
		.origin = buffers.size == 0
			? cig_vec2_zero()
			: cig_vec2_make(cig_element()->absolute_frame.x, cig_element()->absolute_frame.y),
     .clip_frames = INIT_STACK(cig_clip_frame_t)
	};
  
  buffer_element.clip_frames.push(&buffer_element.clip_frames, cig_element()->absolute_frame);
  buffers.push(&buffers, buffer_element);
}

void cig_pop_buffer() {
  CIG_UNUSED(buffers._pop(&buffers));
}

/* ┌─────────────────────┐
───┤  MOUSE INTERACTION  │
   └─────────────────────┘ */
	 
void cig_set_input_state(
	const cig_vec2_t position,
	unsigned int action_mask
) {
	if (input_state.locked == false) {
		input_state._target_prev_frame = input_state._target_this_frame;
		input_state._target_this_frame = 0;
	}
	
	const cig_input_action_type_t previous_action_mask = input_state.action_mask;

	input_state.position = position;
	input_state.action_mask = action_mask;

	input_state.click_state = (!input_state.action_mask && previous_action_mask)
		? (tick - input_state._press_start_tick) < 12
			? ENDED
			: EXPIRED
		: (action_mask & CIG_INPUT_MOUSE_BUTTON_LEFT && !(previous_action_mask & CIG_INPUT_MOUSE_BUTTON_LEFT)) || (action_mask & CIG_INPUT_MOUSE_BUTTON_RIGHT && !(previous_action_mask & CIG_INPUT_MOUSE_BUTTON_RIGHT))
			? input_state.click_state == BEGAN
				? NEITHER
				: BEGAN
			: NEITHER;
		
	if (input_state.action_mask & CIG_INPUT_MOUSE_BUTTON_LEFT && !(previous_action_mask & CIG_INPUT_MOUSE_BUTTON_LEFT)) {
		input_state.last_action_began = CIG_INPUT_MOUSE_BUTTON_LEFT;
	} else if (input_state.action_mask & CIG_INPUT_MOUSE_BUTTON_RIGHT && !(previous_action_mask & CIG_INPUT_MOUSE_BUTTON_RIGHT)) {
		input_state.last_action_began = CIG_INPUT_MOUSE_BUTTON_RIGHT;
	} else {
		input_state.last_action_began = 0;
	}
	
	if (previous_action_mask & CIG_INPUT_MOUSE_BUTTON_LEFT && !(input_state.action_mask & CIG_INPUT_MOUSE_BUTTON_LEFT)) {
		input_state.last_action_ended = CIG_INPUT_MOUSE_BUTTON_LEFT;
	} else if (previous_action_mask & CIG_INPUT_MOUSE_BUTTON_RIGHT && !(input_state.action_mask & CIG_INPUT_MOUSE_BUTTON_RIGHT)) {
		input_state.last_action_ended = CIG_INPUT_MOUSE_BUTTON_RIGHT;
	} else {
		input_state.last_action_ended = 0;
	}

	if (input_state.click_state == BEGAN) {
		input_state._press_start_tick = tick;
		input_state._press_target_id = input_state._target_prev_frame;
	}

	if (input_state.action_mask) {
		if (!input_state.drag.active) {
			input_state.drag.active = true;
			input_state.drag.start_position = input_state.position;
			input_state.drag.change = cig_vec2_zero();
		} else {
			input_state.drag.change = cig_vec2_sub(input_state.position, input_state.drag.start_position);
		}
	} else if (input_state.drag.active) {
		input_state.drag.active = false;
		input_state.drag.change = cig_vec2_zero();
	}
}

cig_input_state_t *cig_input_state() {
	return &input_state;
}

void cig_enable_interaction() {
	cig_element_t *element = cig_element();
	if (element->_interaction_enabled) {
		return;
	}
	element->_interaction_enabled = true;
	handle_element_hover(element);
}

bool cig_hovered() {
	/* See `handle_element_hover` */
	return input_state.locked == false && cig_element()->_interaction_enabled && cig_element()->id == input_state._target_prev_frame;
}

cig_input_action_type_t cig_pressed(
	const cig_input_action_type_t actions,
	const cig_press_flags_t options
) {
	if (!cig_hovered()) {
		return 0;
	}
	
	const cig_input_action_type_t action_mask = actions & input_state.action_mask;
	
	if (action_mask && ((options & CIG_PRESS_INSIDE) == false || (options & CIG_PRESS_INSIDE && input_state._press_target_id == cig_element()->id))) {
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
		if (input_state.click_state == BEGAN && (result = actions & input_state.action_mask)) {
			return result;
		}
	} else {
		if (input_state.click_state == ENDED || (input_state.click_state == EXPIRED && !(options & CIG_CLICK_EXPIRE))) {
			if (options & CIG_CLICK_STARTS_INSIDE && input_state._press_target_id != cig_element()->id) {
				return 0;
			}
			return actions & input_state.last_action_ended;
		}
	}
	
	return 0;
}

/* ┌─────────────┐
───┤  SCROLLING  │
   └─────────────┘ */

bool cig_enable_scroll(cig_scroll_state_t *state) {
  cig_element_t *element = cig_element();
  element->_scroll_state = state ? state : find_scroll_state(element->id);
	cig_enable_clipping();
	return element->_scroll_state != NULL;
}

cig_scroll_state_t* cig_scroll_state() {
	return cig_element()->_scroll_state;
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
	cig_element()->_layout_params.flags &= ~CIG_CULL_SUBFRAMES;
}

void cig_enable_clipping() {
  push_clip(cig_element());
}

void cig_set_next_id(cig_id_t id) {
	next_id = id;
}

unsigned int cig_depth() {
	return cig_element_stack()->size;
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
	const cig_frame_t container, /* Frame into which sub-frames are laid out */
	const cig_frame_t frame, /* Proposed sub-frame, generally from CIG_FILL */
	cig_layout_params_t *prm,
	cig_frame_t *result
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
		if (frame.w == CIG_FILL_CONSTANT) {
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
			w = frame.w;
		}
	} else {
		w = frame.w == CIG_FILL_CONSTANT
			? container.w - prm->_h_pos
			: frame.w;
		
		/* Reset any remaining horizontal positioning in case we modify axis mid-layout */
		prm->_h_pos = 0;
		prm->_h_size = CIG_FILL_CONSTANT;
	}
	
	if (v_axis) {
		if (frame.h == CIG_FILL_CONSTANT) {
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
			h = frame.h;
		}
	} else {
		h = frame.h == CIG_FILL_CONSTANT
			? container.h - prm->_v_pos
			: frame.h;
		
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

	*result = cig_frame_make(x, y, w, h);
	
	return true;
}

/* ╔════════════════════════════════════════════╗
   ║            INTERNAL FUNCTIONS              ║
   ╚════════════════════════════════════════════╝ */

static cig_frame_t resolve_size(const cig_frame_t frame, const cig_element_t *parent) {
  const cig_frame_t content_frame = cig_frame_inset(parent->frame, parent->insets);

	return cig_frame_make(
		frame.x,
		frame.y,
		frame.w == CIG_FILL_CONSTANT ? content_frame.w : frame.w,
		frame.h == CIG_FILL_CONSTANT ? content_frame.h : frame.h
	);
}

static bool next_layout_frame(const cig_frame_t proposed, cig_element_t *parent, cig_frame_t *result) {
	if (parent->_layout_function) {
		return (*parent->_layout_function)(
      cig_frame_inset(parent->frame, parent->insets),
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
	const cig_frame_t frame,
	const cig_insets_t insets,
	cig_layout_params_t params,
	bool (*layout_function)(cig_frame_t, cig_frame_t, cig_layout_params_t*, cig_frame_t*)
) {
	cig_buffer_element_t *current_buffer = buffers._peek(&buffers, 0);
	cig_element_t *top = cig_element();
	
	if (top->_layout_params.limit.total > 0 && top->_layout_params._count.total == top->_layout_params.limit.total) {
		return false;
	}
	
	cig_frame_t next;
	if (!next_layout_frame(frame, top, &next)) {
		top->_id_counter ++;
		return false;
	}
	
  if (top->_scroll_state) {
		top->_scroll_state->content_size.x = CIG_MAX(top->_scroll_state->content_size.x, next.x + next.w);
		top->_scroll_state->content_size.y = CIG_MAX(top->_scroll_state->content_size.y, next.y + next.h);
	  next = cig_frame_offset(next, -top->_scroll_state->offset.x, -top->_scroll_state->offset.y);
  }

	if (top->_layout_params.flags & CIG_CULL_SUBFRAMES
		&& !cig_frame_intersects(top->frame, cig_frame_offset(next, top->frame.x+top->insets.left, top->frame.y+top->insets.top))) {
		top->_id_counter ++;
		return false;
	}

	top->_layout_params._count.total ++;

	cig_frame_t absolute_frame = cig_convert_relative_frame(next);
	cig_frame_t current_clip_frame = current_buffer->clip_frames.peek(&current_buffer->clip_frames, 0);

  elements.push(&elements, (cig_element_t) {
		0,
		.id = next_id ? next_id : (top->id + tinyhash(top->id+top->_id_counter++, cig_depth())),
		.frame = next,
		.clipped_frame = cig_frame_offset(cig_frame_union(absolute_frame, current_clip_frame), -absolute_frame.x + next.x, -absolute_frame.y + next.y),
		.absolute_frame = absolute_frame,
    .insets = insets,
		._layout_function = layout_function,
		._layout_params = params,
    ._clipped = false,
    ._scroll_state = NULL
	});
  
	next_id = 0;

	return true;
}

/*static cig_state_t* im_state(
	const cig_id_t id,
	const imgui_widget_t type
) {
	register size_t i = 0;
	cig_state_t *s;

	for (; i < _state_list.size; ++i) {
		s = &_state_list.values[i];
		if (s->id == id && (type == IM_ANY || type == s->type)) {
			if (tick != s->last_tick) {
				s->activation_state = ACTIVE;
				s->last_tick = tick;
			}
			return s;
		}
	}

	if (_state_list.size == IM_STATES_MAX) {
		exit(66);
	}

	s = &_state_list.values[_state_list.size++];
	s->id = id;
	s->type = type;
	s->internal_record = 0;
	s->activation_state = ACTIVATED;
	s->last_tick = tick;
	memset(&s->data, 0, sizeof(s->data));
	
	return s;
}*/

static void handle_element_hover(const cig_element_t *element) {
	/*
	My approach for mouse hit detection in immediate-mode GUI:
	Due to immediate processing of elements, checking mouse position
	and drawing highlighted/pressed states right away can lead to multiple
	overlapping elements showing those states.
	To address this, we track the most recent (topmost) element the cursor
	touches each frame. On subsequent frame, we compare the currently drawn
	element (ID) with the previous result. This introduces a one-frame delay
	between mouse detection and response but in reality it's imperceptible.
	*/
	if (cig_frame_contains(element->absolute_frame, input_state.position)) {
		input_state._target_this_frame = element->id;
	}
}

static cig_scroll_state_t* find_scroll_state(const cig_id_t id) {
  register int first_available = -1;
	
  for (register int i = 0; i < CIG_SCROLLABLE_ELEMENTS_MAX; ++i) {
    if (_scroll_elements[i].id == id) {
			_scroll_elements[i].last_tick = tick;
      return &_scroll_elements[i].value;
    } else if ((_scroll_elements[i].id == 0 || _scroll_elements[i].last_tick < tick-1) && first_available < 0) {
      first_available = i;
    }
  }
	
  if (first_available < 0) {
    return NULL;
  } else {
    _scroll_elements[first_available].id = id;
    _scroll_elements[first_available].value.offset = cig_vec2_zero();
    _scroll_elements[first_available].value.content_size = cig_vec2_zero();
		_scroll_elements[first_available].last_tick = tick;
		
    return &_scroll_elements[first_available].value;
  }
}

static void push_clip(cig_element_t *element) {
  if (element->_clipped == false) {
		element->_clipped = true;
		stack_cig_clip_frame_t_t *clip_frames = &buffers._peek(&buffers, 0)->clip_frames;
    clip_frames->push(clip_frames, cig_frame_union(element->absolute_frame, clip_frames->peek(clip_frames, 0)));
    
		// Check if graphics module is included
    // TODO: Set new clip region for the renderer
  }
}

static void pop_clip() {
	stack_cig_clip_frame_t_t *clip_frames = &buffers._peek(&buffers, 0)->clip_frames;
  
  CIG_UNUSED(clip_frames->_pop(clip_frames));
  
  // Check if graphics module is included
  // TODO: Set previous clip region for the renderer
}
