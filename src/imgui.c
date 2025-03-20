#include "imgui.h"
#include "imprivt.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>

typedef im_state_t* im_state_ptr_t;

/* Define stack types */
DECLARE_STACK(im_state_ptr_t);
DECLARE_STACK(im_buffer_elements_t);

/* Declare stacks */
static STACK(im_state_ptr_t) widget_states;
static STACK(im_buffer_elements_t) buffers;

/* Internal state members */
static struct {
	im_state_t values[IM_STATES_MAX];
	size_t size;
} _state_list = { 0 };
static im_scroll_state_element_t _scroll_elements[IM_STATES_MAX];
static IMGUIID next_id = 0;
static unsigned int tick = 0;
static struct {
	enum {
		NEITHER,	/* Button was neither pressed or released this frame */
		BEGAN,		/* Button was pressed down this frame (click started) */
		ENDED,		/* Button was released this frame (click ended) */
		EXPIRED		/* Button was held longer than deemed appropriate */
	} click_state;
	struct {
		bool active;
		vec2 start;
		vec2 delta;
	} drag;
	im_mouse_button_t button_mask;
	im_mouse_button_t last_button;
	vec2 position;
	unsigned int press_start_tick;
	unsigned int target_prev_frame;
	unsigned int target_this_frame;
	IMGUIID press_target_id; /* Element that was focused when button press began */
	
	// TODO: ! just use press_target_id and not set it to zero after click !
	IMGUIID last_press_target_id; /* Element that was focused when the button press last began */
	bool locked_on_element;
} mouse = { 0 };

// static im_state_t* 				im_state(const IMGUIID, const imgui_widget_t);

static im_scroll_state_t* get_scroll_state(IMGUIID);

static void handle_element_hover(im_element_t*);

static void im_push_buffer(im_buffer_ref, frame_t);

static void	im_pop_buffer();

static void im_push_clip(im_element_t*);

static void im_pop_clip();

static frame_t resolve_size(frame_t, im_element_t*);

static bool next_layout_frame(frame_t, im_element_t *, frame_t *);

static bool push_frame(frame_t, insets_t, im_layout_params_t, bool (*)(frame_t, frame_t, im_layout_params_t*, frame_t*));

static inline __attribute__((always_inline)) int tinyhash(int a, int b) { return (a * 31) ^ (b * 17); }


/* ┌───────────────┐
───┤  CORE LAYOUT  │
   └───────────────┘ */

void im_begin_layout(const im_buffer_ref buffer, const frame_t frame) {
	STACK_INIT(&widget_states);
	STACK_INIT(&buffers);

	im_push_buffer(buffer, frame);
}

void im_end_layout() {
	register im_state_t *s, *last = NULL;
	
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

void im_set_input_state(
	const vec2 position,
	unsigned int button_mask
) {
	if (mouse.locked_on_element == false) {
		mouse.target_prev_frame = mouse.target_this_frame;
		mouse.target_this_frame = 0;
	}
	
	const im_mouse_button_t previous_button_mask = mouse.button_mask;

	mouse.position = position;
	mouse.button_mask = button_mask;

	mouse.click_state = (!mouse.button_mask && previous_button_mask)
		? (tick - mouse.press_start_tick) < 12
			? ENDED
			: EXPIRED
		: (button_mask & IM_MOUSE_BUTTON_LEFT && !(previous_button_mask & IM_MOUSE_BUTTON_LEFT)) || (button_mask & IM_MOUSE_BUTTON_RIGHT && !(previous_button_mask & IM_MOUSE_BUTTON_RIGHT))
			? BEGAN
			: NEITHER;
			
	if (previous_button_mask & IM_MOUSE_BUTTON_LEFT && !(mouse.button_mask & IM_MOUSE_BUTTON_LEFT)) {
		mouse.last_button = IM_MOUSE_BUTTON_LEFT;
	} else if (previous_button_mask & IM_MOUSE_BUTTON_RIGHT && !(mouse.button_mask & IM_MOUSE_BUTTON_RIGHT)) {
		mouse.last_button = IM_MOUSE_BUTTON_RIGHT;
	} else {
		mouse.last_button = 0;
	}

	if (mouse.click_state == BEGAN) {
		mouse.press_start_tick = tick;
		mouse.press_target_id = mouse.target_prev_frame;
		mouse.last_press_target_id = 0;
	} else if (mouse.click_state == ENDED || mouse.click_state == EXPIRED) {
		mouse.last_press_target_id = mouse.press_target_id;
		mouse.press_target_id = 0;
	}

	if (mouse.button_mask & IM_MOUSE_BUTTON_LEFT) {
		if (!mouse.drag.active) {
			mouse.drag.active = true;
			mouse.drag.start = mouse.position;
			mouse.drag.delta = vec2_zero();
		} else {
			mouse.drag.delta = vec2_sub(mouse.position, mouse.drag.start);
		}
	} else if (mouse.drag.active) {
		mouse.drag.active = false;
		mouse.drag.delta = vec2_zero();
	}
}

void im_reset_internal_state() {
	int i;
	for (i = 0; i < IM_STATES_MAX; ++i) {
    _state_list.values[i].id = 0;
    _scroll_elements[i].id = 0;
  }
	_state_list.size = 0;
	mouse.locked_on_element = false;
	mouse.target_prev_frame = 0;
	mouse.target_this_frame = 0;
	mouse.drag.active = false;
  tick = 0;
	next_id = 0;
}

im_buffer_ref im_get_buffer() {
	return STACK_TOP(&buffers).buffer;
}

bool im_push_frame(const frame_t frame) {
	return push_frame(frame, insets_zero(), (im_layout_params_t){ 0, .options = IM_DEFAULT_LAYOUT_FLAGS }, NULL);
}

bool im_push_frame_insets(
	const frame_t frame,
	const insets_t insets
) {
	return push_frame(frame, insets, (im_layout_params_t){ 0, .options = IM_DEFAULT_LAYOUT_FLAGS }, NULL);
}

bool im_push_frame_insets_params(
	const frame_t frame,
	const insets_t insets,
	const im_layout_params_t params
) {
	return push_frame(frame, insets, params, NULL);
}

bool im_push_frame_function(
	bool (*layout_function)(const frame_t, const frame_t, im_layout_params_t *, frame_t *),
	const frame_t frame,
	const insets_t insets,
	im_layout_params_t params
) {
	return push_frame(frame, insets, params, layout_function);
}

im_element_t* im_pop_frame() {
  im_element_t *popped_element = &STACK_POP(im_get_frame_stack());

  if (popped_element->_clipped) {
    popped_element->_clipped = false;
    im_pop_clip();
  }
  
  return popped_element;
}

im_element_t* im_get_element() {
	return &STACK_TOP(im_get_frame_stack());
}

frame_t im_get_relative_frame() {
	return im_get_element()->frame;
}

frame_t im_get_absolute_frame() {
	return im_get_element()->absolute_frame;
}

frame_t im_convert_relative_frame(const frame_t frame) {
	// TODO: can only use top element's absolute frame to calculate this
	register int i;
	STACK(im_element_t) *frames = im_get_frame_stack();
	frame_t f = resolve_size(frame, &STACK_TOP(frames));
	for (i = frames->size - 1; i >= 0; --i) {
    f.x += frames->elements[i].frame.x + frames->elements[i].insets.left;
    f.y += frames->elements[i].frame.y + frames->elements[i].insets.top;
	}
	return f;
}

STACK(im_element_t)* im_get_frame_stack() {
	return &STACK_TOP(&buffers).elements;
}


/* ┌─────────────────────┐
───┤  MOUSE INTERACTION  │
   └─────────────────────┘ */
	 
void im_enable_interaction() {
	im_element_t *element = im_get_element();
	if (element->_interaction_enabled) {
		return;
	}
	element->_interaction_enabled = true;
	handle_element_hover(element);
}

bool im_hovered() {
	/* See `handle_element_hover` */
	return mouse.locked_on_element == false &&
		im_get_element()->_interaction_enabled &&
		im_get_element()->id == mouse.target_prev_frame;
}

im_mouse_button_t im_pressed(
	const im_mouse_button_t buttons,
	const im_press_options_t options
) {
	if (!im_hovered()) {
		return 0;
	}
	
	const im_mouse_button_t button_mask = buttons & mouse.button_mask;
	
	if (button_mask && ((options & IM_MOUSE_PRESS_INSIDE) == false || (options & IM_MOUSE_PRESS_INSIDE && mouse.press_target_id == im_get_element()->id))) {
		return button_mask;
	} else {
		return 0;
	}
}

im_mouse_button_t im_clicked(
	const im_mouse_button_t buttons,
	const im_click_options_t options
) {
	if (!im_hovered()) {
		return 0;
	}
	
	if (options & IM_CLICK_ON_BUTTON_DOWN) {
		im_mouse_button_t result;
		if (mouse.click_state == BEGAN && (result = buttons & mouse.button_mask)) {
			return result;
		}
	} else {
		if (mouse.click_state == ENDED || (mouse.click_state == EXPIRED && !(options & IM_CLICK_EXPIRE))) {
			if (options & IM_CLICK_STARTS_INSIDE && mouse.last_press_target_id != im_get_element()->id) {
				return 0;
			}
			return buttons & mouse.last_button;
		}
	}
	
	return 0;
}


/* ┌─────────────┐
───┤  SCROLLING  │
   └─────────────┘ */

bool im_enable_scroll(im_scroll_state_t *state) {
  im_element_t *element = im_get_element();
  element->_scroll_state = state ? state : get_scroll_state(element->id);
	im_enable_clipping();
	return element->_scroll_state != NULL;
}

void im_set_scroll(vec2 offset) {
	im_get_scroll_state()->offset = offset;
}

 void im_scroll(vec2 delta) {
	im_get_scroll_state()->offset = vec2_add(im_get_scroll_state()->offset, delta);
}

vec2 im_get_scroll() {
	return im_get_scroll_state()->offset;
}

im_scroll_state_t* im_get_scroll_state() {
	return im_get_element()->_scroll_state;
}


/* ┌──────────────────┐
───┤  LAYOUT HELPERS  │
   └──────────────────┘ */
	 
void im_disable_culling() {
	im_get_element()->_layout_params.options &= ~IM_CULL_SUBFRAMES;
}

void im_enable_clipping() {
  im_push_clip(im_get_element());
}

void im_set_next_id(IMGUIID id) {
	next_id = id;
}

unsigned int im_get_depth() {
	return STACK_SIZE(im_get_frame_stack());
}

/* http://www.cse.yorku.ca/~oz/hash.html */
IMGUIID im_hash(const char *str) {
	register IMGUIID hash = 5381;
	register int c;
	while ((c = *str++)) {
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
	}
	return hash;
}

void im_empty() {
	im_push_frame(IM_FILL);
	im_pop_frame();
}

void im_spacer(const int size) {
	im_push_frame(IM_FILL_H(size));
	im_pop_frame();
}

bool im_default_layout_builder(
	const frame_t container, /* Frame into which sub-frames are laid out */
	const frame_t frame, /* Proposed sub-frame, generally from IM_FILL */
	im_layout_params_t *prm,
	frame_t *result
) {
	const bool h_axis = prm->axis & HORIZONTAL;
	const bool v_axis = prm->axis & VERTICAL;
	const bool is_grid = h_axis && v_axis;
	
	int x = prm->_horizontal_position,
	    y = prm->_vertical_position,
			w,
			h;

	void move_to_next_row() {
		prm->_horizontal_position = 0;
		prm->_vertical_position += (prm->_v_size + prm->spacing);
		prm->_h_size = IM_FILL_CONSTANT;
		prm->_v_size = IM_FILL_CONSTANT;
		prm->_count.h_cur = 0;
	}
	
	void move_to_next_column() {
		prm->_vertical_position = 0;
		prm->_horizontal_position += (prm->_h_size + prm->spacing);
		prm->_h_size = IM_FILL_CONSTANT;
		prm->_v_size = IM_FILL_CONSTANT;
		prm->_count.v_cur = 0;
	}
	
	if (h_axis) {
		if (frame.w == IM_FILL_CONSTANT) {
			if (prm->width > 0) {
				w = prm->width;
			} else if (prm->columns) {
				w = (container.w - ((prm->columns - 1) * prm->spacing)) / prm->columns;
			} else if (prm->_h_size && prm->direction == DIR_DOWN) {
				w = prm->_h_size;
			}	else {
				w = container.w - prm->_horizontal_position;
			}
		} else {
			w = frame.w;
		}
	} else {
		w = frame.w == IM_FILL_CONSTANT
			? container.w - prm->_horizontal_position
			: frame.w;
		
		/* Reset any remaining horizontal positioning in case we modify axis mid-layout */
		prm->_horizontal_position = 0;
		prm->_h_size = IM_FILL_CONSTANT;
	}
	
	if (v_axis) {
		if (frame.h == IM_FILL_CONSTANT) {
			if (prm->height > 0) {
				h = prm->height;
			} else if (prm->rows) {
				h = (container.h - ((prm->rows - 1) * prm->spacing)) / prm->rows;
			} else if (prm->_v_size && prm->direction == DIR_LEFT) {
				h = prm->_v_size;
			}	else {
				h = container.h - prm->_vertical_position;
			}
		} else {
			h = frame.h;
		}
	} else {
		h = frame.h == IM_FILL_CONSTANT
			? container.h - prm->_vertical_position
			: frame.h;
		
		/* Reset any remaining vertical positioning in case we modify axis mid-layout */
		prm->_vertical_position = 0;
		prm->_v_size = IM_FILL_CONSTANT;
	}
	
	if (h_axis && v_axis) {
		/* Can we fit the new frame onto current axis? */
		switch (prm->direction) {
			case DIR_LEFT: {
				if ((prm->limit.horizontal && prm->_count.h_cur == prm->limit.horizontal) || prm->_horizontal_position + w > container.w) {
					move_to_next_row();
					x = 0;
					y = prm->_vertical_position;
				}
				
				prm->_horizontal_position += (w + prm->spacing);
				prm->_h_size = MAX(prm->_h_size, w);
				prm->_v_size = MAX(prm->_v_size, h);
				prm->_count.h_cur ++;
				
				if (prm->_horizontal_position >= container.w) {
					move_to_next_row();
				}
			} break;
			case DIR_DOWN: {
				if ((prm->limit.vertical && prm->_count.v_cur == prm->limit.vertical) || prm->_vertical_position + h > container.h) {
					move_to_next_column();
					y = 0;
					x = prm->_horizontal_position;
				}
				
				prm->_vertical_position += (h + prm->spacing);
				prm->_h_size = MAX(prm->_h_size, w);
				prm->_v_size = MAX(prm->_v_size, h);
				prm->_count.v_cur ++;
				
				if (prm->_vertical_position >= container.h) {
					move_to_next_column();
				}
			} break;
			default: break;
		}
	} else if (h_axis) {
		if (prm->limit.horizontal && prm->_count.h_cur == prm->limit.horizontal) {
			return false;
		}
		prm->_horizontal_position += (w + prm->spacing);
		prm->_h_size = MAX(prm->_h_size, w);
		prm->_count.h_cur ++;
	} else if (v_axis) {
		if (prm->limit.vertical && prm->_count.v_cur == prm->limit.vertical) {
			return false;
		}
		prm->_vertical_position += (h + prm->spacing);
		prm->_v_size = MAX(prm->_v_size, h);
		prm->_count.v_cur ++;
	}

	*result = frame_make(x, y, w, h);
	
	return true;
}


/* ╔════════════════════════════════════════════╗
   ║            INTERNAL FUNCTIONS              ║
   ╚════════════════════════════════════════════╝ */

static frame_t resolve_size(const frame_t frame, im_element_t *parent) {
  const frame_t content_frame = frame_inset(parent->frame, parent->insets);

	return frame_make(
		frame.x,
		frame.y,
		frame.w == IM_FILL_CONSTANT ? content_frame.w : frame.w,
		frame.h == IM_FILL_CONSTANT ? content_frame.h : frame.h
	);
}

static bool next_layout_frame(
	const frame_t proposed_frame,
	im_element_t *top_element,
	frame_t *result
) {
	if (top_element->_layout_function) {
		return (*top_element->_layout_function)(
      frame_inset(top_element->frame, top_element->insets),
      proposed_frame,
      &top_element->_layout_params,
			result
    );
	} else {
		*result = resolve_size(proposed_frame, top_element);
		return true;
	}
}

static bool push_frame(
	const frame_t frame,
	const insets_t insets,
	im_layout_params_t params,
	bool (*layout_function)(frame_t, frame_t, im_layout_params_t*, frame_t*)
) {
	im_buffer_elements_t *current_buffer = &STACK_TOP(&buffers);
	im_element_t *top = &STACK_TOP(&current_buffer->elements);
	
	if (top->_layout_params.limit.total > 0 && top->_layout_params._count.total == top->_layout_params.limit.total) {
		return false;
	}
	
	frame_t next;
	if (!next_layout_frame(frame, top, &next)) {
		top->_id_counter ++;
		return false;
	}
	
  if (top->_scroll_state) {
		top->_scroll_state->content_size.x = MAX(top->_scroll_state->content_size.x, next.x + next.w);
		top->_scroll_state->content_size.y = MAX(top->_scroll_state->content_size.y, next.y + next.h);
	  next = frame_offset(next, -top->_scroll_state->offset.x, -top->_scroll_state->offset.y);
  }

	if (top->_layout_params.options & IM_CULL_SUBFRAMES
		&& !frame_intersects(top->frame, frame_offset(next, top->frame.x+top->insets.left, top->frame.y+top->insets.top))) {
		top->_id_counter ++;
		return false;
	}

	top->_layout_params._count.total ++;

	frame_t absolute_frame = im_convert_relative_frame(next);
	frame_t current_clip_frame = STACK_TOP(&current_buffer->clip_frames);

	STACK_PUSH(&current_buffer->elements, ((im_element_t){
		0,
		.id = next_id ? next_id : (top->id + tinyhash(top->id+top->_id_counter++, im_get_depth())),
		.frame = next,
		.clipped_frame = frame_offset(frame_union(absolute_frame, current_clip_frame), -absolute_frame.x + next.x, -absolute_frame.y + next.y),
		.absolute_frame = absolute_frame,
    .insets = insets,
		._layout_function = layout_function,
		._layout_params = params,
    ._clipped = false,
    ._scroll_state = NULL
	}));
	
	next_id = 0;

	return true;
}


/*static im_state_t* im_state(
	const IMGUIID id,
	const imgui_widget_t type
) {
	register size_t i = 0;
	im_state_t *s;

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

static void handle_element_hover(im_element_t *element) {
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
	if (frame_contains(element->absolute_frame, mouse.position)) {
		mouse.target_this_frame = element->id;
	}
}

static im_scroll_state_t* get_scroll_state(const IMGUIID id) {
  register int first_available = -1;
	
  for (register int i = 0; i < IM_STATES_MAX; ++i) {
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
    _scroll_elements[first_available].value.offset = vec2_zero();
    _scroll_elements[first_available].value.content_size = vec2_zero();
		_scroll_elements[first_available].last_tick = tick;
		
    return &_scroll_elements[first_available].value;
  }
}

static void im_push_buffer(const im_buffer_ref buffer, const frame_t frame) {
	im_buffer_elements_t buffer_framestack;
	buffer_framestack.buffer = buffer;
	
	STACK_INIT(&buffer_framestack.elements);
	STACK_INIT(&buffer_framestack.clip_frames);
	
	STACK_PUSH(&buffer_framestack.elements, ((im_element_t) {
		0,
		.id = next_id ? next_id : (im_hash("buffer") + STACK_SIZE(&buffers) << 8),
		.frame = frame,
		.clipped_frame = frame,
		.absolute_frame = frame,
		.insets = insets_zero(),
		._layout_function = NULL,
		._layout_params = (im_layout_params_t){ 0, .options = IM_DEFAULT_LAYOUT_FLAGS }
	}));
	
	STACK_PUSH(&buffer_framestack.clip_frames, frame);
	
	next_id = 0;
	
	STACK_PUSH(&buffers, buffer_framestack);
}

static void	im_pop_buffer() {
	STACK_POP_NORETURN(&buffers);
}

static void im_push_clip(im_element_t *element) {
  if (element->_clipped == false) {
		element->_clipped = true;
		STACK(frame_t) *clip_frames = &STACK_TOP(&buffers).clip_frames;
    STACK_PUSH(clip_frames, frame_union(element->absolute_frame, STACK_TOP(clip_frames)));

		// Check if graphics module is included
		/*if (backend.set_clip) {
			(*backend.set_clip)(buffer, frame);
    }*/
  }
}

static void im_pop_clip() {
	STACK(frame_t) *clip_frames = &STACK_TOP(&buffers).clip_frames;
  STACK_POP_NORETURN(clip_frames);

  // Go back to previous clip if present
  if (STACK_IS_EMPTY(clip_frames)) {
		/*if (backend.reset_clip) {
			(*backend.reset_clip)(imgui_get_buffer());
		}*/
  } else {
		/*if (backend.set_clip) {
			const im_clip_element_t *top = &STACK_TOP(&clip_frames);
			(*backend.set_clip)(top->buffer, top->frame);
		}*/
  }
}
