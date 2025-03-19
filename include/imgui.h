#ifndef IMGUI_H
#define IMGUI_H

#include "imlimits.h"
#include "imguim.h"
// #include "types.h"
#include "stack.h"
#include "types/frame.h"
#include "types/insets.h"
// #include "backend/keys.h"
#include <stdbool.h>
#include <stddef.h>

#define IM_FILL_CONSTANT 0
#define IM_FILL frame_make(0, 0, IM_FILL_CONSTANT, IM_FILL_CONSTANT)
#define IM_FILL_W(W) frame_make(0, 0, W, IM_FILL_CONSTANT)
#define IM_FILL_H(H) frame_make(0, 0, IM_FILL_CONSTANT, H)
#define IM_X im_current_element()->frame.x
#define IM_Y im_current_element()->frame.y
#define IM_W im_current_element()->frame.w
#define IM_H im_current_element()->frame.h
#define IM_CENTER_WH(W, H) frame_make((IM_W * 0.5) - (W * 0.5), (IM_H * 0.5) - (H * 0.5), W, H)
#define IM_CENTER vec2_make((IM_W * 0.5), (IM_H * 0.5))
#define IM_R (IM_X + IM_W)
#define IM_B (IM_Y + IM_H)
#define CIM_SCROLL_LIMIT_X im_current_element()->_scroll_state->content_size.x-im_current_element()->frame.w+im_current_element()->insets.left+im_current_element()->insets.right
#define CIM_SCROLL_LIMIT_Y im_current_element()->_scroll_state->content_size.y-im_current_element()->frame.h+im_current_element()->insets.top+im_current_element()->insets.bottom

#define IM_INLINE inline __attribute__((always_inline))

typedef unsigned long IMGUIID;

typedef void* im_buffer_ref;
typedef void* im_image_ref;
typedef void* im_font_ref;
typedef int im_color_ref;
typedef int im_keycode;
typedef unsigned char im_char_t;

typedef struct {
	short line_height, line_spacing, baseline_offset;
} im_font_info_t;

typedef enum {
  IM_TEXT_ALIGN_LEFT = 0,
  IM_TEXT_ALIGN_CENTER,
  IM_TEXT_ALIGN_RIGHT
} im_text_horizontal_alignment_t;

typedef enum {
  IM_TEXT_ALIGN_TOP = 0,
  IM_TEXT_ALIGN_MIDDLE,
  IM_TEXT_ALIGN_BOTTOM
} im_text_vertical_alignment_t;

#define IM_API_DRAW_IMAGE(F) void (F)( \
	const im_buffer_ref buffer, \
	const im_image_ref image, \
	const frame_t frame, \
	const bool flip_horizontally, \
	const bool flip_vertically, \
	const bool center \
)

#define IM_API_DRAW_IMAGE_REGION(F) void (F)( \
	const im_buffer_ref buffer, \
	const im_image_ref image, \
	const frame_t frame, \
	const vec2 offset \
)

#define IM_API_DRAW_LINE(F) void (F)( \
	const im_buffer_ref buffer, \
	const int x1, \
	const int y1, \
	const int x2, \
	const int y2, \
	const im_color_ref color \
)

#define IM_API_DRAW_RECT(F) void (F)( \
	const im_buffer_ref buffer, \
	const int x, \
	const int y, \
	const int w, \
	const int h, \
	const im_color_ref fill, \
	const im_color_ref stroke \
)

#define IM_API_DRAW_TEXT(F) void (F)( \
	const im_buffer_ref buffer, \
	const int x, \
	const int y, \
	const im_font_ref font, \
	const im_text_horizontal_alignment_t alignment, \
	const im_color_ref color, \
	const char *text \
)

#define IM_API_GET_TEXT_SIZE(F) int (F)( \
	const im_font_ref font, \
	const char *string \
)

#define IM_API_GET_FONT_INFO(F) im_font_info_t (F)(const im_font_ref font)

#define IM_API_GET_NEXT_INPUT_CHARACTER(F) im_char_t (F)()

#define IM_API_ALLOCATE_BUFFER(F) im_buffer_ref (F)(const int w, const int h)
#define IM_API_CLEAR_BUFFER(F) void (F)(const im_buffer_ref buffer)
#define IM_API_BLIT_BUFFER(F) void (F)( \
	const im_buffer_ref dst, \
	const im_buffer_ref src, \
	const frame_t frame \
)
#define IM_API_RELEASE_BUFFER(F) void (F)(im_buffer_ref buffer)

#define IM_API_SET_CLIP(N) void (N)(im_buffer_ref buffer, const frame_t frame)
#define IM_API_RESET_CLIP(N) void (N)(im_buffer_ref buffer)

typedef enum {
	IM_ANY = 0,
	IM_MENUBAR,
	IM_MENU,
	IM_LIST,
	IM_WINDOW,
	IM_TEXT,
	IM_INPUT,
	IM_SLIDER,
  IM_SCROLLER
} imgui_widget_t;

typedef struct im_multiline_text_line_t {
	size_t index;
	size_t length;
	char *begin;
	char *end;
} im_multiline_text_line_t;

typedef struct im_multiline_text_info_t {
	char *str;
	unsigned char number_of_lines;
	size_t length;
	im_multiline_text_line_t lines[IM_MULTILINE_LINES_MAX];
	vec2 bounds;
} im_multiline_text_info_t;

typedef enum {
	BITFLAG(0, IM_CULL_SUBFRAMES),
	
	IM_DEFAULT_LAYOUT_FLAGS = IM_CULL_SUBFRAMES
} im_layout_options_t;

typedef enum {
	CASEFLAG(0, IM_MOUSE_BUTTON_LEFT),
	CASEFLAG(1, IM_MOUSE_BUTTON_RIGHT),
	IM_MOUSE_BUTTON_ANY = IM_MOUSE_BUTTON_LEFT | IM_MOUSE_BUTTON_RIGHT
} im_mouse_button_t;

/*
 * Type containing parameters passed to layout function.
 */
typedef struct im_layout_params_t {
	enum {
		CASEFLAG(1, HORIZONTAL),
		CASEFLAG(2, VERTICAL),
	} axis;
	enum {
		CASE(0, DIR_LEFT),
		CASE(1, DIR_DOWN)
	} direction; /* Direction in which the layout flows.
									Only applies to grids (axis = VERTICAL | HORIZONTAL) */
	int spacing, width, height, columns, rows;
	struct {
		int horizontal, vertical, total;
	} limit;
	im_layout_options_t options;
	void *data;
	/* Private. Keep out! */
	int _horizontal_position, _vertical_position, _h_size, _v_size;
	struct {
		int h_cur, v_cur, total; /* h_ and v_cur are only counted in stacks/grids */
	} _count;
} im_layout_params_t;

typedef struct im_scroll_state_t {
  vec2 offset;
	vec2 content_size;
} im_scroll_state_t;

typedef struct frame_stack_element_t {
	IMGUIID id;
	frame_t frame;
	frame_t absolute_frame;
  insets_t insets;
	/* Private. Keep out! */
	bool (*_layout_function)(const frame_t, const frame_t, im_layout_params_t *, frame_t *);
	im_layout_params_t _layout_params;
  bool _clipped;
  im_scroll_state_t *_scroll_state;
	unsigned int _id_counter;
	struct {
		bool hovered, clicked;
	} _input_state;
} frame_stack_element_t;

DECLARE_STACK(frame_stack_element_t);

STACK(frame_stack_element_t)* im_frame_stack();

typedef struct {
	im_buffer_ref buffer;
	STACK(frame_stack_element_t) frames;
} im_buffer_framestack_t;

typedef struct im_backend_configuration_t {
	IM_API_DRAW_IMAGE(*draw_image);
	IM_API_DRAW_IMAGE_REGION(*draw_image_region);
	IM_API_DRAW_LINE(*draw_line);
	IM_API_DRAW_RECT(*draw_rect);
	IM_API_DRAW_TEXT(*draw_text);
	IM_API_GET_TEXT_SIZE(*get_text_size);
	IM_API_GET_FONT_INFO(*get_font_info);
	IM_API_GET_NEXT_INPUT_CHARACTER(*get_next_input_character);
	IM_API_ALLOCATE_BUFFER(*allocate_buffer);
	IM_API_CLEAR_BUFFER(*clear_buffer);
	IM_API_BLIT_BUFFER(*blit_buffer);
	IM_API_RELEASE_BUFFER(*release_buffer);
  IM_API_SET_CLIP(*set_clip);
  IM_API_RESET_CLIP(*reset_clip);
} im_backend_configuration_t;

void im_configure(const im_backend_configuration_t config);

void im_begin_layout(const im_buffer_ref, const frame_t);

void im_end_layout();

void imgui_reset_internal_state();

void im_set_input_state(const vec2, unsigned int);

im_buffer_ref imgui_get_buffer();

frame_stack_element_t* im_current_element();

// Pop last frame element from the stack
frame_stack_element_t* im_pop_frame();

// Current local space frame
frame_t im_relative_frame();

// Current screen space frame
frame_t im_absolute_frame();

//
frame_t imgui_root_frame();

/* Convert a relative frame to an absolute frame */
frame_t im_convert_relative_frame(const frame_t);

void im_next_id(IMGUIID);

unsigned int im_depth();

IM_INLINE bool im_hovered() {
	return im_current_element()->_input_state.hovered;
}

typedef enum {
	CASEFLAG(0, IM_MOUSE_PRESS_INSIDE)
} im_pressed_options_t;

bool im_pressed(const im_mouse_button_t, const im_pressed_options_t);

// Fills current frame with selected fill style
void imgui_fill_panel(const int, const short);
void im_fill_color(im_color_ref);
void im_stroke_color(im_color_ref);

void im_draw_rect(const int x, const int y, const int w, const int h, const im_color_ref fill, im_color_ref stroke);
void im_draw_line(const int x1, const int y1, const int x2, const int y2, const im_color_ref color);

// V2

/* -----------------------------------------------------------------------------
* Configuration stacks
*/

typedef enum {
	IM_PANEL = 0,
	IM_TEXT_BASELINE_OFFSET,
	IM_BUTTON_NORMAL_TEXT_COLOR, // TODO: read from panel style?
	IM_BUTTON_PRESSED_TEXT_COLOR,
	IM_TEXT_COLOR,
	IM_FONT
} im_option_key_t;

typedef union {
	im_color_ref color;
	int i;
	void *ref;
} im_value_t;

#define __IM_OPTION_KEY_COUNT 16
#define IM_VALUE_COLOR(V) (im_value_t){ .color = V }
#define IM_VALUE_INT(V) (im_value_t){ .i = V }
#define IM_VALUE_REF(V) (im_value_t){ .ref = V }

void im_push_value(const im_option_key_t, const im_value_t);

im_value_t im_pop_value(const im_option_key_t);

im_color_ref im_color_value(const im_option_key_t k);
int im_int_value(const im_option_key_t k);
void* im_ref_value(const im_option_key_t k);

#define IM_REF(TYPE, K) ((TYPE)im_ref_value(K))

/* -----------------------------------------------------------------------------
 * ((( Buttons )))
 */

void im_set_default_font(im_font_ref);

/* -----------------------------------------------------------------------------
 * Input building blocks.
 */

typedef struct {
	short index;
	bool hover;
	bool press;
	bool click;
	vec2 position;
} im_mouse_result_t;

typedef enum {
	BITFLAG(0, _IM_MOUSE_PRESS_INSIDE),
	BITFLAG(1, IM_MOUSE_CLICK_ON_PRESS),
	BITFLAG(2, IM_MOUSE_NO_CONVERT),
	BITFLAG(3, IM_MOUSE_CLICK_EXPIRE)
} im_mouse_listener_option_t;

im_mouse_button_t im_mouse_listener(
	IMGUIID id,
	const frame_t frame,
	const unsigned int flags,
	const im_mouse_button_t buttons,
	bool *hovered,
	bool *pressed,
	vec2 *mouse_position
);

void im_key_handler(void (*handler)(const im_keycode));

bool im_key(const im_keycode key);
bool im_key_repeat(const im_keycode key, const int rate);

/* -----------------------------------------------------------------------------
* Misc. & utils
*/

/*
 * Pushes and pops an empty frame to trigger a layout function
 * to allocate space. Useful when you have a stack or grid and
 * want to add empty space.
 */
void im_empty();

void im_separator();

void im_insert_spacer(const int size);

/*
 *
 */
IMGUIID im_hash(const char *str);

/* ((( Layout functions and convenience elements related to that ))) */

/* Push a new frame with zero insets to layout stack.
 * @return TRUE if frame is visible within current container, FALSE otherwise. */
bool im_push_frame(const frame_t);

/* Push a new frame with custom insets to layout stack.
 * @return TRUE if frame is visible within current container, FALSE otherwise. */
bool im_push_frame_insets(const frame_t frame, const insets_t insets);

/* Push a new frame with custom insets and params to layout stack.
 * @return TRUE if frame is visible within current container, FALSE otherwise. */
bool im_push_frame_insets_params(const frame_t frame, const insets_t insets, const im_layout_params_t);

/* Push layout builder function to layout stack.
 * @return TRUE if frame is visible within current container, FALSE otherwise. */
bool im_push_frame_builder(
	const frame_t frame,
	const insets_t insets,
	bool (*layout_function)(
		const frame_t,
		const frame_t,
		im_layout_params_t *,
		frame_t *
	),
	im_layout_params_t
);

bool im_stack_layout_builder(
	const frame_t container,
	const frame_t frame,
	im_layout_params_t *params,
	frame_t *result
);

void im_enable_scroll(im_scroll_state_t *);
void im_enable_clip();
void im_disable_culling();

#ifdef DEBUG

void im_debug_log(const char *);

#define IM_PRINT(...) { char print_buffer[128]; sprintf(print_buffer, __VA_ARGS__); im_debug_log(print_buffer); }

#else

#define IM_PRINT(...)

#endif

#endif
