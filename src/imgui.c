#include "imgui.h"
#include "imprivt.h"
// #include "backend/backend.h"
// #include "gfx.h"
// #include "input.h"
// #include "res.h"
// #include "game.h"
// #include "macros.h"
// #include "memory.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <math.h>

typedef im_state_t* im_state_ptr_t;

// Define stack types
DECLARE_STACK(im_state_ptr_t);
DECLARE_STACK(im_value_t);
DECLARE_STACK(im_buffer_framestack_t);
DECLARE_STACK(im_clip_element_t);

// Declare stacks
static STACK(im_state_ptr_t) widget_states;
static STACK(im_value_t) options[__IM_OPTION_KEY_COUNT];
static STACK(im_buffer_framestack_t) buffers;
static STACK(im_clip_element_t) clip_frames;

static im_backend_configuration_t backend = { NULL };

static struct {
	im_state_t values[IM_STATES_MAX];
	size_t size;
} _state_list = { 0 };

static im_scroll_state_element_t _scroll_elements[IM_STATES_MAX];

static struct {
	enum {
		NEITHER,	/* Button was neither pressed or released this frame */
		BEGAN,		/* Button was pressed down this frame (click started) */
		ENDED,		/* Button was released this frame (click ended) */
		FAILED		/* Button was held longer than deemed appropriate */
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
	bool locked_on_element;
} mouse = { 0 };

static struct {
	IMGUIID keycode_target[2][IM_KEYCODE_COUNT];
	bool any_consumer;
} keyboard = { 0 };

static IMGUIID next_id = 0;
static unsigned int tick = 0, caret_tick = 0;
static im_font_ref default_font = NULL;

#ifdef DEBUG
struct {
	int visibility;
	char custom_msg[80];
	int max_frame_depth;
	int total_frames;
	bool current_buffer_updated;
} debug_info = { .visibility = 0 };
#endif

// V2

static char __strbuf128[128];
static struct {
	im_font_ref ref;
	im_font_info_t info;
	short line_height_total;
} cached_font;

static im_state_t* 	im_state(const IMGUIID, const imgui_widget_t);
static im_scroll_state_t* im_scroll_state(const IMGUIID);
static bool					im_state_record_value(im_state_t*, int);
static IMGUIID 			im_frame_identity(const frame_t);
static void 				cache_font(const im_font_ref);
static void 				handle_element_interaction(frame_stack_element_t *);
static void 				im_push_buffer(const im_buffer_ref, const frame_t);
static void					im_pop_buffer();
static void					im_blit_buffer(im_buffer_ref);
static void         im_push_clip(frame_stack_element_t *);
static void         im_pop_clip();

IM_INLINE STACK(frame_stack_element_t)* im_frame_stack() {
	return &STACK_TOP(&buffers).frames;
}

void im_configure(const im_backend_configuration_t config) {
	backend = config;
}

void im_begin_layout(const im_buffer_ref buffer, const frame_t frame) {
	register int i;

#ifdef DEBUG
	debug_info.max_frame_depth = 1;
	debug_info.total_frames = 0;
	debug_info.current_buffer_updated = false;
#endif

	STACK_INIT(&widget_states);
	STACK_INIT(&buffers);
  STACK_INIT(&clip_frames);

	/*if (backend.clear_buffer) {
    (*backend.clear_buffer)(buffer);
  }*/
	
	im_push_buffer(buffer, frame);

	for (i = 0; i < __IM_OPTION_KEY_COUNT; ++i) {
		STACK_INIT(&options[i]);
	}
	
	// Push defaults
	im_push_value(IM_PANEL, IM_VALUE_INT(0));
	im_push_value(IM_TEXT_BASELINE_OFFSET, IM_VALUE_INT(0));
	// im_push_value(IM_BUTTON_NORMAL_TEXT_COLOR, IM_VALUE_COLOR(COLOR_RED_DARK));
	// im_push_value(IM_BUTTON_PRESSED_TEXT_COLOR, IM_VALUE_COLOR(COLOR_RED_DARK));
	// im_push_value(IM_TEXT_COLOR, IM_VALUE_COLOR(COLOR_BLACK));
	im_push_value(IM_FONT, IM_VALUE_REF(default_font));

#ifdef DEBUG
	/*if (input_key_modifier(K_MODIFIER_SHIFT) && input_key(K_D)) {
		debug_info.visibility = (debug_info.visibility + 1) % 3;
	}*/
#endif
}

void im_end_layout() {
	register int i;
	register im_state_t *s, *last = NULL;
	
	++tick;
	++caret_tick;

	for (i = _state_list.size - 1; i >= 0; --i) {
		s = &_state_list.values[i];
		if (s->last_tick != tick) {
			if (last) {
				*s = *last;
				last = NULL;
			} else {
				s->id = 0;
				s->activation_state = INACTIVE;
				if (s->buffer) {
					(*backend.release_buffer)(s->buffer);
					s->buffer = NULL;
				}
			}
			_state_list.size --;
		} else if (!last) {
			last = s;
		}
	}
	
	/*if (!keyboard.any_consumer) {
		// Drain key buffer
		while ((*backend.get_next_input_character)()) {}
	}*/

#ifdef DEBUG
	/*if (debug_info.visibility > 0) {
		char debug_buf[150];
		const int dy = 200 - cached_font.info.line_height;
		sprintf(debug_buf, "(%d)S=%ld|Fmax=%d,Ftot=%d|Mb=%d|%s",
			debug_info.visibility,
			_state_list.size, debug_info.max_frame_depth, debug_info.total_frames,
			mouse.button_mask,
			debug_info.custom_msg
		);
		const int text_size = (*backend.get_text_size)(cached_font.ref, debug_buf);
		im_draw_rect(0, dy, text_size, cached_font.info.line_height, COLOR_BLACK, COLOR_NONE);
		(*backend.draw_text)(imgui_get_buffer(), 0, dy + cached_font.info.baseline_offset, cached_font.ref, TEXT_ALIGN_LEFT, COLOR_FUCHSIA, debug_buf);
	}*/
#endif
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
			: FAILED
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
	} else if (mouse.click_state == ENDED || mouse.click_state == FAILED) {
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

void imgui_reset_internal_state() {
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

im_buffer_ref imgui_get_buffer() {
	return STACK_TOP(&buffers).buffer;
}

// Frame stack

FASTFUNC frame_t frame_resolve_size(
	const frame_t frame,
	frame_stack_element_t *parent
) {
  const frame_t content_frame = frame_inset(parent->frame, parent->insets);

	return frame_make(
		frame.x,
		frame.y,
		frame.w == IM_FILL_CONSTANT ? content_frame.w : frame.w,
		frame.h == IM_FILL_CONSTANT ? content_frame.h : frame.h
	);
}

FASTFUNC bool next_layout_frame(
	const frame_t proposed_frame,
	frame_stack_element_t *top_element,
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
		*result = frame_resolve_size(proposed_frame, top_element);
		return true;
	}
}

/* Frame pushing */

static unsigned int tinyhash(int a, int b) {
	return (a * 31) ^ (b * 17);
}

FASTFUNC bool _im_push_frame(
	const frame_t frame,
	const insets_t insets,
	bool (*layout_function)(const frame_t, const frame_t, im_layout_params_t *, frame_t *),
	im_layout_params_t params
) {
	frame_stack_element_t *top = &STACK_TOP(im_frame_stack());
	
	if (top->_layout_params.limit.total > 0 && top->_layout_params._count.total == top->_layout_params.limit.total) {
		return false;
	}
	
	frame_t next;
	if (!next_layout_frame(frame, top, &next)) {
		top->_id_counter ++;
		return false;
	}
	
  if (top->_scroll_state) {
    next = frame_offset(next, -top->_scroll_state->offset.x, -top->_scroll_state->offset.y);
		top->_scroll_state->content_size.x = MAX(top->_scroll_state->content_size.x, next.x + next.w);
		top->_scroll_state->content_size.y = MAX(top->_scroll_state->content_size.y, next.y + next.h);
  }

	if (top->_layout_params.options & IM_CULL_SUBFRAMES
		&& !frame_intersects(top->frame, frame_offset(next, top->frame.x+top->insets.left, top->frame.y+top->insets.top))) {
		top->_id_counter ++;
		return false;
	}

	top->_layout_params._count.total ++;
	
	frame_stack_element_t element = {
		.id = next_id ? next_id : (top->id + tinyhash(top->id+top->_id_counter++, im_depth())),
		.frame = next,
		.absolute_frame = im_convert_relative_frame(next),
    .insets = insets,
		._layout_function = layout_function,
		._layout_params = params,
    ._clipped = false,
    ._scroll_state = NULL
	};
	
	handle_element_interaction(&element);

	STACK_PUSH(im_frame_stack(), element);
	
	next_id = 0;

#ifdef DEBUG
	debug_info.total_frames ++;
	debug_info.max_frame_depth = MAX(debug_info.max_frame_depth, STACK_SIZE(im_frame_stack()));
#endif

	return true;
}

bool im_push_frame(const frame_t frame) {
	return _im_push_frame(frame, insets_zero(), NULL, (im_layout_params_t){ 0, .options = IM_DEFAULT_LAYOUT_FLAGS });
}

bool im_push_frame_insets(
	const frame_t frame,
	const insets_t insets
) {
	return _im_push_frame(frame, insets, NULL, (im_layout_params_t){ 0, .options = IM_DEFAULT_LAYOUT_FLAGS });
}

bool im_push_frame_insets_params(
	const frame_t frame,
	const insets_t insets,
	const im_layout_params_t params
) {
	return _im_push_frame(frame, insets, NULL, params);
}

bool im_push_frame_builder(
	const frame_t frame,
	const insets_t insets,
	bool (*layout_function)(const frame_t, const frame_t, im_layout_params_t *, frame_t *),
	im_layout_params_t params
) {
	return _im_push_frame(frame, insets, layout_function, params);
}

/* ----- */


frame_stack_element_t* im_pop_frame() {
#ifdef DEBUG
	frame_stack_element_t *top = &STACK_TOP(im_frame_stack());
	if (debug_info.visibility > 1 && top->_layout_function == &im_stack_layout_builder) {
		int remaining_space = top->_layout_params.axis == VERTICAL
			? (top->frame.h - MAX(0, (top->_layout_params._vertical_position - top->_layout_params.spacing)))
			: (top->frame.w - MAX(0, (top->_layout_params._horizontal_position - top->_layout_params.spacing)));
		if (remaining_space > 0) {
			char numbuf[8];
			sprintf(numbuf, "%d", remaining_space);
			im_push_frame(top->_layout_params.axis == VERTICAL
				? IM_FILL_H(remaining_space)
				: IM_FILL_W(remaining_space));
			im_fill_color(0);
			// const vec2 c = frame_center(im_absolute_frame());
			// (*backend.draw_text)(imgui_get_buffer(), c.x, c.y-(cached_font.info.line_height*0.5), cached_font.ref, TEXT_ALIGN_CENTER, COLOR_BLACK, numbuf);
			top->_layout_function = NULL;
			im_pop_frame();
		}
	}
#endif
  frame_stack_element_t *popped_element = &STACK_POP(im_frame_stack());

  if (popped_element->_clipped) {
    popped_element->_clipped = false;
    im_pop_clip();
  }
  
  return popped_element;
}

frame_stack_element_t* im_current_element() {
	return &STACK_TOP(im_frame_stack());
}

/* Current relative frame */
frame_t im_relative_frame() {
	return im_current_element()->frame;
}

/* Current screen-space frame */
frame_t im_absolute_frame() {
	return im_current_element()->absolute_frame;
}

frame_t imgui_root_frame() {
	return buffers.elements[0].frames.elements[0].frame;
}

// Convert a relative/local frame to screen-space
frame_t im_convert_relative_frame(const frame_t frame) {
	register int i;
	STACK(frame_stack_element_t) *frames = im_frame_stack();
	frame_t f = frame_resolve_size(frame, &STACK_TOP(frames));
	for (i = frames->size - 1; i >= 0; --i) {
    f.x += frames->elements[i].frame.x + frames->elements[i].insets.left;
    f.y += frames->elements[i].frame.y + frames->elements[i].insets.top;
	}
	return f;
}

void im_next_id(IMGUIID id) {
	next_id = id;
}

unsigned int im_depth() {
	return STACK_SIZE(im_frame_stack());
}

// ------

void imgui_fill_panel( const int style, const short flags) {
 	/*frame_t f = im_absolute_frame(); // im_convert_relative_frame(IM_FILL);
	const bool pressed = (bool)(flags & IMGUIPANELPRESSED);
	if (pressed && flags & IMGUIPANELPRESSADJUST) { f.y += 1; f.h -= 1; }
	image_t *image = &panel_cache[style].image[pressed];

	const bool tl = (bool)(flags & IMGUIPANELTL);
	const bool tr = (bool)(flags & IMGUIPANELTR);
	const bool bl = (bool)(flags & IMGUIPANELBL);
	const bool br = (bool)(flags & IMGUIPANELBR);
	
	im_buffer_ref buf = imgui_get_buffer();

	// Center fill
	{
		const int x1 = f.x + 8;
		const int x2 = f.x + f.w - 8;
		const int y1 = f.y + 8;
		const int y2 = f.y + f.h - 8;

		if (x2 - x1 > 0 && y2 - y1 > 0) {
			buffer_draw_rect_correct(buf, frame_make(x1, y1, x2-x1, y2-y1), COLOR_NONE, panel_cache[style].fill_color);
		}
	}

	// Left section
	{
		const int y1 = f.y + (tl ? 8 : 0);
		const int y2 = f.y + f.h - (bl ? 8 : 0);
		if (y2 - y1 > 0) {
			buffer_draw_rect_correct(buf, frame_make(f.x+2, y1, 7, y2-y1), COLOR_NONE, panel_cache[style].fill_color);
			if (flags & IMGUIPANELL) {
				buffer_draw_line(buf, vec2_make(f.x, y1), vec2_make(f.x, y2-1), panel_cache[style].left_color[0]);
				buffer_draw_line(buf, vec2_make(f.x+1, y1), vec2_make(f.x+1, y2-1), panel_cache[style].left_color[1]);
			}
		}
	}

	// Right section
	{
		const int y1 = f.y + (tr ? 8 : 0);
		const int y2 = f.y + f.h - (br ? 8 : 0);
		const int xw = f.x + f.w - 1;
		if (y2 - y1 > 0) {
			buffer_draw_rect_correct(buf, frame_make(xw-7, y1, 7, y2-y1), COLOR_NONE, panel_cache[style].fill_color);
			if (flags & IMGUIPANELR) {
				buffer_draw_line(buf, vec2_make(xw, y1), vec2_make(xw, y2-1), panel_cache[style].right_color[0]);
				buffer_draw_line(buf, vec2_make(xw-1, y1), vec2_make(xw-1, y2-1), panel_cache[style].right_color[1]);
			}
		}
	}

	// Top section
	{
		const int x1 = f.x + (tl ? 8 : 0);
		const int x2 = f.x + f.w - (tr ? 8 : 0);
		if (x2 - x1 > 0) {
			buffer_draw_rect_correct(buf, frame_make(x1, f.y, x2-x1, 8), COLOR_NONE, panel_cache[style].fill_color);
			if (flags & IMGUIPANELT) {
				buffer_draw_line(buf, vec2_make(x1, f.y), vec2_make(x2-1, f.y), panel_cache[style].top_color[0]);
				buffer_draw_line(buf, vec2_make(x1, f.y+1), vec2_make(x2-1, f.y+1), panel_cache[style].top_color[1]);
			}
		}
		if (tl) { buffer_blit_image(buf, image, 0, 0, f.x, f.y, 8, 8, true, NULL); }
		if (tr) { buffer_blit_image(buf, image, 9, 0, x2, f.y, 8, 8, true, NULL); }
	}

	// Bottom section
	{
		const int x1 = f.x + (bl ? 8 : 0);
		const int x2 = f.x + f.w - (br ? 8 : 0);
		const int y = f.y + f.h - 8;

		if (x2 - x1 > 0) {
			buffer_draw_rect_correct(buf, frame_make(x1, y, x2-x1, 8), COLOR_NONE, panel_cache[style].fill_color);
			if (flags & IMGUIPANELB) {
				buffer_draw_line(buf, vec2_make(x1, y+7), vec2_make(x2-1, y+7), panel_cache[style].bottom_color[pressed][1]);
				buffer_draw_line(buf, vec2_make(x1, y+6), vec2_make(x2-1, y+6), panel_cache[style].bottom_color[pressed][0]);
			}
		}
		if (bl) { buffer_blit_image(buf, image, 0, 9, f.x, y, 8, 8, true, NULL); }
		if (br) { buffer_blit_image(buf, image, 9, 9, x2, y, 8, 8, true, NULL); }
	}*/
}

void im_fill_color(im_color_ref color) {
	const frame_t f = im_absolute_frame();
	im_draw_rect(f.x, f.y, f.w, f.h, color, -1);
}

void im_stroke_color(im_color_ref color) {
	const frame_stack_element_t *f = im_current_element();
	
	im_draw_rect(f->absolute_frame.x, f->absolute_frame.y, f->absolute_frame.w, f->absolute_frame.h, -1, color);
	
	
	// const frame_t f = im_absolute_frame();
	// im_draw_rect(f.x, f.y, f.w, f.h, -1, color);
}

void im_draw_rect(const int x, const int y, const int w, const int h, const im_color_ref fill, im_color_ref stroke) {
	(*backend.draw_rect)(imgui_get_buffer(), x, y, w, h, fill, stroke);
}

void im_draw_line(const int x1, const int y1, const int x2, const int y2, const im_color_ref color) {
	(*backend.draw_line)(imgui_get_buffer(), x1, y1, x2, y2, color);
}

// Private

static void handle_element_interaction(frame_stack_element_t *element) {
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

		if (mouse.locked_on_element == false && element->id == mouse.target_prev_frame) {
			element->_input_state.hovered = true;
		}
	} else {
		element->_input_state.hovered = false;
		element->_input_state.pressed = false;
	}
	
#ifdef OLDM
	if (frame_contains(screen_frame, mouse.position)) {
		mouse.target_this_frame = id;

		if (mouse.locked_on_element == false && id == mouse.target_prev_frame) {
			if (hovered) { *hovered = true; }
			if (pressed) {
				/*
				 * If `IM_MOUSE_PRESS_INSIDE` option is specified, the press has start
				 * within the bounds of this element. Otherwise it can start
				 * outside and element reflects pressed state as soon as mouse
				 * moves onto it.
				 */
				*pressed = buttons & mouse.button_mask && (
					(flags & IM_MOUSE_PRESS_INSIDE) == false || (flags & IM_MOUSE_PRESS_INSIDE && mouse.press_target_id == id)
				);
			}

			/*
			 * A click is detected either when mouse button is first pressed down,
			 * or when it's released, depending on the option.
			 */
			if (flags & IM_MOUSE_CLICK_ON_PRESS) {
				im_mouse_button_t result;
				if (mouse.click_state == BEGAN && (result = buttons & mouse.button_mask)) {
					return result;
				}
			} else {
				im_mouse_button_t result;
				if ((mouse.click_state == ENDED || (mouse.click_state == FAILED && !(flags & IM_MOUSE_CLICK_EXPIRE))) && (result = buttons & mouse.last_button)) {
					return result;
				}
			}

			return 0;
		}
	}
#endif
}


// V2

void im_push_value(const im_option_key_t k, const im_value_t o) {
	STACK_PUSH(&options[k], o);
	if (k == IM_FONT) { cache_font(o.ref); }
}

im_value_t im_pop_value(const im_option_key_t k) {
	im_value_t top = STACK_POP(&options[k]);
	if (k == IM_FONT) { cache_font(im_ref_value(k)); }
	return top;
}

im_color_ref im_color_value(const im_option_key_t k) {
	return STACK_TOP(&options[k]).color;
}

int im_int_value(const im_option_key_t k) {
	return STACK_TOP(&options[k]).i;
}

void* im_ref_value(const im_option_key_t k) {
	return STACK_TOP(&options[k]).ref;
}


/* -----------------------------------------------------------------------------
* Buttons!
*/

void im_set_default_font(im_font_ref font) {
	default_font = font;
}

/*
 * ((( Input handling )))
 */

static bool im_key_id(
	const IMGUIID id,
	const im_keycode key,
	const int rate
) {
	keyboard.keycode_target[0][key] = id;

	return keyboard.keycode_target[1][key] == id; // && input_key_repeat(key, rate);
}

static IMGUIID im_frame_identity(const frame_t frame) {
	return (((31313 ^ (frame.x + 1)) << 5) + ((107007 ^ (frame.y+1)) << 16));
}

im_mouse_button_t im_mouse_listener(
	IMGUIID id,
	const frame_t frame,
	const unsigned int flags,
	const im_mouse_button_t buttons,
	bool *hovered,
	bool *pressed,
	vec2 *mouse_position
) {
	/* Convert relative frame to screen space */
	const frame_t screen_frame = (flags & IM_MOUSE_NO_CONVERT)
		? frame
		: im_convert_relative_frame(frame);

	/* Calculate mouse position relative to current origin */
	if (mouse_position) {
		*mouse_position = vec2_sub(mouse.position, vec2_make(screen_frame.x, screen_frame.y));
	}

	/*
	 * My approach for mouse hit detection in immediate-mode GUI:
	 * Due to immediate processing of elements, checking mouse position
	 * and drawing highlighted/pressed states right away can lead to multiple
	 * overlapping elements showing those states.
	 * To address this, we track the most recent (topmost) element the cursor
	 * touches each frame. On subsequent frame, we compare the currently drawn
	 * element (ID) with the previous result. This introduces a one-frame delay
	 * between mouse detection and response but in reality it's imperceptible.
	 */
	if (frame_contains(screen_frame, mouse.position)) {
		mouse.target_this_frame = id;

		if (mouse.locked_on_element == false && id == mouse.target_prev_frame) {
			if (hovered) { *hovered = true; }
			if (pressed) {
				/*
				 * If `IM_MOUSE_PRESS_INSIDE` option is specified, the press has start
				 * within the bounds of this element. Otherwise it can start
				 * outside and element reflects pressed state as soon as mouse
				 * moves onto it.
				 */
				*pressed = buttons & mouse.button_mask && (
					(flags & IM_MOUSE_PRESS_INSIDE) == false || (flags & IM_MOUSE_PRESS_INSIDE && mouse.press_target_id == id)
				);
			}

			/*
			 * A click is detected either when mouse button is first pressed down,
			 * or when it's released, depending on the option.
			 */
			if (flags & IM_MOUSE_CLICK_ON_PRESS) {
				im_mouse_button_t result;
				if (mouse.click_state == BEGAN && (result = buttons & mouse.button_mask)) {
					return result;
				}
			} else {
				im_mouse_button_t result;
				if ((mouse.click_state == ENDED || (mouse.click_state == FAILED && !(flags & IM_MOUSE_CLICK_EXPIRE))) && (result = buttons & mouse.last_button)) {
					return result;
				}
			}

			return 0;
		}
	}

	if (hovered) { *hovered = false; }
	if (pressed) { *pressed = false; }

	return 0;
}

bool im_key(const im_keycode key) {
	return im_key_repeat(key, 0);
}

bool im_key_repeat(const im_keycode key, const int rate) {
	const im_state_t *top_state = STACK_TOP(&widget_states);
	const IMGUIID id = (top_state ? top_state->id : 0)
		+ im_frame_identity(im_relative_frame())
	 	+ (key << 8);

	return im_key_id(id, key, rate);
}

void im_key_handler(void (*handler)(const im_keycode)) {
	const im_state_t *top_state = STACK_TOP(&widget_states);
	const IMGUIID id = (top_state ? top_state->id : 0)
		+ im_frame_identity(im_relative_frame())
	 	+ (int)handler;

	size_t i;

	for (i = 0; i < IM_KEYCODE_COUNT; ++i) {
		if (im_key_id(id + (i << 8), (im_keycode)i, 0)) {
			if (handler) {
				(*handler)((im_keycode)i);
			}
		}
	}
}

/* -----------------------------------------------------------------------------
 * Misc.
 */

void im_empty() {
	/*
	 * Trigger currently top layout function to allocate the next frame.
	 * If a regular frame is currently at top, this call does nothing.
	 */
	im_push_frame(IM_FILL);
	im_pop_frame();
}

/* http://www.cse.yorku.ca/~oz/hash.html */
ALWAYS_INLINED IMGUIID im_hash(const char *str) {
	register IMGUIID hash = 5381;
	register int c;
	while ((c = *str++)) {
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
	}
	return hash;
}

ALWAYS_INLINED void im_separator() {
	// IM_ARRANGE(frame_make(0, (IM_H * 0.5), IM_W, 1), imgui_fill_color(COLOR_GRAY, COLOR_NONE))
}

ALWAYS_INLINED void im_insert_spacer(const int size) {
	IM_ARRANGE(IM_FILL_H(size), IM_BODY());
}

/* -----------------------------------------------------------------------------
* Layout functions and convenience elements related to that.
*/

bool im_stack_layout_builder(
	/* Frame into which sub-frames are laid out */
	const frame_t container,
	/* Proposed sub-frame, generally from IM_FILL */
	const frame_t frame,
	im_layout_params_t *prm,
	frame_t *result
) {
	const bool h_axis = prm->axis & HORIZONTAL;
	const bool v_axis = prm->axis & VERTICAL;
	const bool is_grid = h_axis && v_axis;
	
	int x = prm->_horizontal_position, y = prm->_vertical_position, w, h;

	int resolve_width() {
		return frame.w == IM_FILL_CONSTANT
			? container.w - prm->_horizontal_position
			: frame.w;
	}
	
	int resolve_height() {
		return frame.h == IM_FILL_CONSTANT
			? container.h - prm->_vertical_position
			: frame.h;
	}
	
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

/*
 * Scroller
 */

void im_enable_scroll(im_scroll_state_t *state) {
  frame_stack_element_t *current_frame = im_current_element();
  current_frame->_scroll_state = state ? state : im_scroll_state(current_frame->id);
  im_enable_clip();
}

void im_enable_clip() {
  im_push_clip(im_current_element());
}

void im_disable_culling() {
	im_current_element()->_layout_params.options &= ~IM_CULL_SUBFRAMES;
}


/* -----------------------------------------------------------------------------
* Internal functions
*/

/*
 * Find or allocate state buffer for given ID and widget type.
 * If IM_ANY is passed as type, first state with the ID is returned,
 * otherwise both the identifier and type have to match.
 */
static im_state_t* im_state(
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
}

static bool	im_state_record_value(
	im_state_t *state,
	int value
) {
	if (state->internal_record != value) {
		state->internal_record = value;
		return true;
	}
	return false;
}

static im_scroll_state_t* im_scroll_state(const IMGUIID id) {
  register int i, first_available = -1;
  for (i = 0; i < IM_STATES_MAX; ++i) {
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

static void cache_font(const im_font_ref font) {
	if (!font) { return; }
	cached_font.ref = font;
	cached_font.info = (*backend.get_font_info)(font);
	cached_font.line_height_total = cached_font.info.line_height + cached_font.info.line_spacing;
}

static void im_push_buffer(const im_buffer_ref buffer, const frame_t frame) {
	im_buffer_framestack_t buffer_framestack;
	buffer_framestack.buffer = buffer;
	
	STACK_INIT(&buffer_framestack.frames);
	STACK_PUSH(&buffer_framestack.frames, ((frame_stack_element_t) {
		0,
		.id = next_id ? next_id : (im_hash("buffer") + STACK_SIZE(&buffers) << 8),
		.frame = frame,
		.absolute_frame = frame,
		.insets = insets_zero(),
		._layout_function = NULL,
		._layout_params = (im_layout_params_t){ 0, .options = IM_DEFAULT_LAYOUT_FLAGS }
	}));
	
	next_id = 0;
	
	STACK_PUSH(&buffers, buffer_framestack);
}

static void	im_pop_buffer() {
	STACK_POP_NORETURN(&buffers);
}

static void im_blit_buffer(im_buffer_ref src) {
	im_buffer_ref dst = imgui_get_buffer();
	if (dst == src) { return; }
	const frame_t f = im_absolute_frame();
	(*backend.blit_buffer)(dst, src, f);
#ifdef DEBUG
	if(debug_info.visibility > 0 && debug_info.current_buffer_updated) {
		debug_info.current_buffer_updated = false;
		// (*backend.draw_rect)(dst, f.x, f.y, f.w, f.h, COLOR_NONE, COLOR_FUCHSIA);
	}
#endif
}

static bool find_last_frame(
  im_buffer_ref *buffer,
  frame_t *result
) {
  register int i;
  im_clip_element_t *element;
  for (i = 0; i < STACK_SIZE(&clip_frames); ++i) {
    element = &STACK_AT(&clip_frames, i);
    if (element->buffer == buffer) {
      *result = element->frame;
      return true;
    }
  }
  return false;
}

static void im_push_clip(frame_stack_element_t *frame) {
  if (frame->_clipped == false) {
    frame->_clipped = true;
    im_buffer_ref *buffer = imgui_get_buffer();
    frame_t frame = im_absolute_frame();
    frame_t last;

    if (find_last_frame(buffer, &last)) {
      frame = frame_union(frame, last);
    }    

    (*backend.set_clip)(buffer, frame);
    
    STACK_PUSH(&clip_frames, ((im_clip_element_t) {
      .buffer = buffer,
      .frame = frame
    }));
  }
}

static void im_pop_clip() {
  STACK_POP_NORETURN(&clip_frames);

  // Go back to previous clip if present
  if (STACK_IS_EMPTY(&clip_frames)) {
    (*backend.reset_clip)(imgui_get_buffer());
  } else {
    const im_clip_element_t *top = &STACK_TOP(&clip_frames);
    (*backend.set_clip)(top->buffer, top->frame);
  }
}

#ifdef DEBUG

void im_debug_log(const char *msg) {
  if (!msg) { return; }
  FILE *f = fopen("imlog.txt", "a");
  if (f) {
    fputs(msg, f);
    fputs("\n", f);
    // fflush(log_file);
    fclose(f);
  }
}

#endif
