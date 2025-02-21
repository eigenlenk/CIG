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

// Declare stacks
static STACK(im_state_ptr_t) widget_states;
static STACK(im_value_t) options[__IM_OPTION_KEY_COUNT];
static STACK(im_buffer_framestack_t) buffers;

static im_backend_configuration_t backend = { NULL };

static struct {
	im_state_t values[IM_STATES_MAX];
	size_t size;
} _state_list;

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
} mouse;

static struct {
	IMGUIID keycode_target[2][IM_KEYCODE_COUNT];
	bool any_consumer;
} keyboard;

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
static bool					im_state_record_value(im_state_t*, int);
static IMGUIID 			im_frame_identity(const frame_t);
static frame_t 			frame_apply_insets(const frame_t, const insets_t);
static void 				cache_font(const im_font_ref);
static void 				im_push_buffer(const im_buffer_ref, const frame_t);
static void					im_pop_buffer();
static void					im_blit_buffer(im_buffer_ref);

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

	++tick;
	++caret_tick;

	if (mouse.locked_on_element == false) {
		mouse.target_prev_frame = mouse.target_this_frame;
		mouse.target_this_frame = 0;
	}

	const im_mouse_button_t previous_button_mask = mouse.button_mask;

	/*mouse.position = input_position();
	mouse.button_mask = input_mouse_button_held(MOUSE_BUTTON_LEFT, NULL) ? IM_MOUSE_BUTTON_LEFT : 0
		| input_mouse_button_held(MOUSE_BUTTON_RIGHT, NULL) ? IM_MOUSE_BUTTON_RIGHT : 0;
	mouse.click_state = (!mouse.button_mask && previous_button_mask)
		? (tick - mouse.press_start_tick) < 12
			? ENDED
			: FAILED
		: input_click(MOUSE_BUTTON_LEFT) || input_click(MOUSE_BUTTON_RIGHT)
			? BEGAN
			: NEITHER;*/

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

	memcpy(&keyboard.keycode_target[1][0], &keyboard.keycode_target[0][0], IM_KEYCODE_COUNT * sizeof(IMGUIID));
	memset(&keyboard.keycode_target[0][0], 0, IM_KEYCODE_COUNT * (sizeof(IMGUIID) / sizeof(char)));
	
	keyboard.any_consumer = false;

#ifdef DEBUG
	/*if (input_key_modifier(K_MODIFIER_SHIFT) && input_key(K_D)) {
		debug_info.visibility = (debug_info.visibility + 1) % 3;
	}*/
#endif
}

void im_end_layout() {
	register int i = _state_list.size - 1;
	register im_state_t *s, *last = NULL;

	for (; i >= 0; --i) {
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
	
	if (!keyboard.any_consumer) {
		// Drain key buffer
		while ((*backend.get_next_input_character)()) {}
	}

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

void imgui_reset_internal_state() {
	int i;
	for (i = 0; i < IM_STATES_MAX; ++i) { _state_list.values[i].id = 0; }
	_state_list.size = 0;
	mouse.locked_on_element = false;
	mouse.target_prev_frame = 0;
	mouse.target_this_frame = 0;
	mouse.drag.active = false;
  tick = 0;
}

im_buffer_ref imgui_get_buffer() {
	return STACK_TOP(&buffers).buffer;
}

// Frame stack

FASTFUNC frame_t frame_resolve_size(
	const frame_t frame,
	const insets_t insets,
	frame_stack_element_t *top_element
) {
	return frame_make(
		frame.x + insets.left,
		frame.y + insets.top,
		(frame.w == IM_FILL_CONSTANT ? top_element->frame.w : frame.w) - (insets.left + insets.right),
		(frame.h == IM_FILL_CONSTANT ? top_element->frame.h : frame.h) - (insets.top + insets.bottom)
	);
}

FASTFUNC frame_t next_layout_frame(
	const frame_t proposed_frame,
	const insets_t insets,
	frame_stack_element_t *top_element
) {
	if (top_element->layout_function) {
		return (*top_element->layout_function)(top_element->frame, proposed_frame, insets, &top_element->layout_params);
	} else {
		return frame_resolve_size(proposed_frame, insets, top_element);
	}
}

/* Frame pushing */

FASTFUNC bool _im_push_frame(
	const frame_t frame,
	const insets_t insets,
	frame_t (*layout_function)(const frame_t, const frame_t, const insets_t, im_layout_params_t *),
	im_layout_params_t params
) {
	frame_stack_element_t *top = &STACK_TOP(im_frame_stack());
	const frame_t next = next_layout_frame(frame, insets, top);

	if (top->layout_params.options & IM_CLIP_SUBFRAMES && !frame_wholly_contains_relative_frame(top->frame, next)) {
		return false;
	}

	STACK_PUSH(im_frame_stack(), ((frame_stack_element_t) {
		.id = top->id + im_frame_identity(next),
		.frame = next,
		.layout_function = layout_function,
		.layout_params = params
	}));

#ifdef DEBUG
	debug_info.total_frames ++;
	debug_info.max_frame_depth = MAX(debug_info.max_frame_depth, STACK_SIZE(im_frame_stack()));
#endif

	return true;
}

bool im_push_frame(const frame_t frame) {
	return _im_push_frame(frame, insets_zero(), NULL, (im_layout_params_t){0});
}

bool im_push_frame_insets(
	const frame_t frame,
	const insets_t insets
) {
	return _im_push_frame(frame, insets, NULL, (im_layout_params_t){0});
}

bool im_push_layout_frame_insets(
	const frame_t frame,
	const insets_t insets,
	frame_t (*layout_function)(const frame_t, const frame_t, const insets_t, im_layout_params_t *),
	im_layout_params_t params
) {
	return _im_push_frame(frame, insets, layout_function, params);
}

/* ----- */


frame_stack_element_t* im_pop_frame() {
#ifdef DEBUG
	frame_stack_element_t *top = &STACK_TOP(im_frame_stack());
	if (debug_info.visibility > 1 && top->layout_function == &im_layout_stack) {
		int remaining_space = top->layout_params.axis == VERTICAL
			? (top->frame.h - MAX(0, (top->layout_params.vertical_position - top->layout_params.spacing)))
			: (top->frame.w - MAX(0, (top->layout_params.horizontal_position - top->layout_params.spacing)));
		if (remaining_space > 0) {
			char numbuf[8];
			sprintf(numbuf, "%d", remaining_space);
			im_push_frame(top->layout_params.axis == VERTICAL
				? IM_FILL_H(remaining_space)
				: IM_FILL_W(remaining_space));
			imgui_fill_color(COLOR_FUCHSIA, COLOR_NONE);
			const vec2 c = frame_center(imgui_current_screen_space_frame());
			(*backend.draw_text)(imgui_get_buffer(), c.x, c.y-(cached_font.info.line_height*0.5), cached_font.ref, TEXT_ALIGN_CENTER, COLOR_BLACK, numbuf);
			top->layout_function = NULL;
			im_pop_frame();
		}
	}
#endif
	return &STACK_POP(im_frame_stack());
}

// Current local space frame
frame_t imgui_current_frame() {
	return STACK_TOP(im_frame_stack()).frame;
}

frame_stack_element_t* im_current_frame() {
	return &STACK_TOP(im_frame_stack());
}

// Current screen space frame
frame_t imgui_current_screen_space_frame() {
	register int i;
	STACK(frame_stack_element_t) *frames = im_frame_stack();
	frame_t f = STACK_TOP(frames).frame;
	for (i = frames->size - 2; i >= 0; --i) {
		f.x += frames->elements[i].frame.x;
		f.y += frames->elements[i].frame.y;
	}
	return f;
}

frame_t imgui_root_frame() {
	return buffers.elements[0].frames.elements[0].frame;
}

// Convert a relative/local frame to screen-space
frame_t imgui_frame_convert(const frame_t frame) {
	register int i;
	STACK(frame_stack_element_t) *frames = im_frame_stack();
	frame_t f = frame_resolve_size(frame, insets_zero(), &STACK_TOP(frames));
	for (i = frames->size - 1; i >= 0; --i) {
		f.x += frames->elements[i].frame.x;
		f.y += frames->elements[i].frame.y;
	}
	return f;
}

// ------

void imgui_fill_panel( const int style, const short flags) {
 	/*frame_t f = imgui_current_screen_space_frame(); // imgui_frame_convert(IM_FILL);
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

void im_fill_color(im_color_ref fill) {
	const frame_t f = imgui_current_screen_space_frame();
	im_draw_rect(f.x, f.y, f.w, f.h, fill, 0);
}

void im_draw_rect(const int x, const int y, const int w, const int h, const im_color_ref fill, im_color_ref stroke) {
	(*backend.draw_rect)(imgui_get_buffer(), x, y, w, h, fill, stroke);
}

void im_draw_line(const int x1, const int y1, const int x2, const int y2, const im_color_ref color) {
	(*backend.draw_line)(imgui_get_buffer(), x1, y1, x2, y2, color);
}

// Private


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
	return (((33333 ^ (frame.x + 1)) << 5) + ((107007 ^ (frame.y+1)) << 7)) << ((frame.w ^ frame.h) % 16);
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
		: imgui_frame_convert(frame);

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
		+ im_frame_identity(imgui_current_frame())
	 	+ (key << 8);

	return im_key_id(id, key, rate);
}

void im_key_handler(void (*handler)(const im_keycode)) {
	const im_state_t *top_state = STACK_TOP(&widget_states);
	const IMGUIID id = (top_state ? top_state->id : 0)
		+ im_frame_identity(imgui_current_frame())
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

static frame_t frame_apply_insets(
	const frame_t frame,
	const insets_t insets
) {
	return frame_make(
		frame.x + insets.left,
		frame.y + insets.top,
		frame.w - (insets.left + insets.right),
		frame.h - (insets.top + insets.bottom)
	);
}

frame_t im_stack_layout_builder(
	/* Frame into which sub-frames are laid out */
	const frame_t container,
	/* Proposed sub-frame, generally from IM_FILL, so it's the same size as `container` */
	const frame_t frame,
	const insets_t insets,
	im_layout_params_t *params
) {
	const bool haxis = params->axis & HORIZONTAL;
	const bool vaxis = params->axis & VERTICAL;

	frame_t result = frame_make(
		haxis ? params->horizontal_position : 0,
		vaxis ? params->vertical_position : 0,
		vaxis && frame.w == IM_FILL_CONSTANT ? container.w : frame.w,
		haxis && frame.h == IM_FILL_CONSTANT ? container.h : frame.h
	);

	/*
	* If `IM_EQUALLY_ALONG_AXIS` option is specified, width or height
	* argument of `imgui_layout_params_t` is used to divide container bounds
	* into that number of even sections, rather than use these values as
	* absolute sizes for these sections. This applies to either or both axis
	* provided in the params.
	*/

	if (haxis) {
		if (params->options & IM_EQUALLY_ALONG_AXIS) {
			result.w = (container.w - ((params->width - 1) * params->spacing)) / params->width;
		} else if (params->width > 0) {
			result.w = params->width;
		} else if (frame.w == IM_FILL_CONSTANT) {
			result.w = container.w - (params->horizontal_position % container.w);
		} else if (frame.w < 0) {
			result.h = (container.w + frame.w) - params->horizontal_position;
		}	else {
			result.w = frame.w;
		}
		params->horizontal_position += (result.w + params->spacing);
	}

	if (vaxis) {
		if (params->options & IM_EQUALLY_ALONG_AXIS) {
			result.h = (container.h - ((params->height - 1) * params->spacing)) / params->height;
		} else if (params->height > 0) {
			result.h = params->height;
		} else if (frame.h == IM_FILL_CONSTANT) {
			result.h = container.h - (params->vertical_position % container.h);
		} else if (frame.h < 0) {
			result.h = (container.h + frame.h) - params->vertical_position;
		}	else {
			result.h = frame.h;
		}
		/*
		 * Normally, vertical axis grows with each new element, but if horizontal
		 * axis is also enabled, that axis first has to be filled before it
		 * jumps to the next line.
		 */
		if (haxis) {
			if (params->horizontal_position >= container.w) {
				params->horizontal_position = 0;
				params->vertical_position += (result.h + params->spacing);
			}
		} else {
			params->vertical_position += (result.h + params->spacing);
		}
	}

	params->_count ++;

	return frame_apply_insets(result, insets);
}

/*
 * Scroller
 */

void im_make_scrollable() {
  frame_stack_element_t *current_frame = im_current_frame();
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

static void cache_font(const im_font_ref font) {
	cached_font.ref = font;
	cached_font.info = (*backend.get_font_info)(font);
	cached_font.line_height_total = cached_font.info.line_height + cached_font.info.line_spacing;
}

static void im_push_buffer(const im_buffer_ref buffer, const frame_t frame) {
	im_buffer_framestack_t buffer_framestack;
	buffer_framestack.buffer = buffer;
	
  if (backend.clear_buffer) {
    (*backend.clear_buffer)(buffer);
  }
	
	STACK_INIT(&buffer_framestack.frames);
	STACK_PUSH(&buffer_framestack.frames, ((frame_stack_element_t) {
		.id = im_frame_identity(frame),
		.frame = frame,
		.layout_function = NULL
	}));
	
	STACK_PUSH(&buffers, buffer_framestack);
}

static void	im_pop_buffer() {
	STACK_POP_NORETURN(&buffers);
}

static void im_blit_buffer(im_buffer_ref src) {
	im_buffer_ref dst = imgui_get_buffer();
	if (dst == src) { return; }
	const frame_t f = imgui_current_screen_space_frame();
	(*backend.blit_buffer)(dst, src, f);
#ifdef DEBUG
	if(debug_info.visibility > 0 && debug_info.current_buffer_updated) {
		debug_info.current_buffer_updated = false;
		(*backend.draw_rect)(dst, f.x, f.y, f.w, f.h, COLOR_NONE, COLOR_FUCHSIA);
	}
#endif
}
