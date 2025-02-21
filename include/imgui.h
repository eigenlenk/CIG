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
#define IM_W imgui_current_frame().w
#define IM_H imgui_current_frame().h
#define IM_X imgui_current_frame().x
#define IM_Y imgui_current_frame().y
#define IM_CENTER_WH(W, H) frame_make((IM_W * 0.5) - (W * 0.5), (IM_H * 0.5) - (H * 0.5), W, H)
#define IM_CENTER vec2_make((IM_W * 0.5), (IM_H * 0.5))
#define IM_R (imgui_current_frame().x + imgui_current_frame().w)
#define IM_B (imgui_current_frame().y + imgui_current_frame().h)

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
	/*
	 * IM_EQUALLY_ALONG_AXIS
	 * Width or height argument of `imgui_layout_params_t` is used
	 * to divide container bounds into even sections, rather than use
	 * these numbers as absolute sizes for these sections.
	 * This applies to either or both axis provided in the params.
	 */
	BITFLAG(0, IM_EQUALLY_ALONG_AXIS),
	BITFLAG(1, IM_CLIP_SUBFRAMES)
} im_layout_options_t;

/*
 * Type containing parameters passed to layout function.
 *
 * width, height - See `IM_EQUALLY_ALONG_AXIS`
 */
typedef struct {
	enum {
		HORIZONTAL = 1,
		VERTICAL = 2
	} axis;
	int spacing, vertical_position, horizontal_position, width, height;
	im_layout_options_t options;
	void *data;
	int _count;
} im_layout_params_t;

typedef struct {
	IMGUIID id;
	frame_t frame;
	frame_t (*layout_function)(const frame_t, const frame_t, const insets_t, im_layout_params_t *);
	im_layout_params_t layout_params;
} frame_stack_element_t;

DECLARE_STACK(frame_stack_element_t);

IM_INLINE STACK(frame_stack_element_t)* im_frame_stack();

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
} im_backend_configuration_t;

void im_configure(const im_backend_configuration_t config);

void imgui_prepare_resources();

void im_begin_layout(const im_buffer_ref, const frame_t);
void im_end_layout();

void imgui_reset_internal_state();

im_buffer_ref imgui_get_buffer();

// Pop last frame element from the stack
frame_stack_element_t* im_pop_frame();

// Current local space frame
frame_t imgui_current_frame();

// Current screen space frame
frame_t imgui_current_screen_space_frame();

//
frame_t imgui_root_frame();

//
frame_t imgui_frame_convert(const frame_t);

// Fills current frame with selected fill style
void imgui_fill_panel(const int, const short);
void im_fill_color(im_color_ref);

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
	BITFLAG(0, IM_MOUSE_PRESS_INSIDE),
	BITFLAG(1, IM_MOUSE_CLICK_ON_PRESS),
	BITFLAG(2, IM_MOUSE_NO_CONVERT),
	BITFLAG(3, IM_MOUSE_CLICK_EXPIRE)
} im_mouse_listener_option_t;

typedef enum {
	BITFLAG(0, IM_MOUSE_BUTTON_LEFT),
	BITFLAG(1, IM_MOUSE_BUTTON_RIGHT)
} im_mouse_button_t;

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

/* Push layout builder function to layout stack.
 * @return TRUE if frame is visible within current container, FALSE otherwise. */
bool im_push_layout_frame_insets(
	const frame_t frame,
	const insets_t insets,
	frame_t (*layout_function)(
		const frame_t,
		const frame_t,
		const insets_t,
		im_layout_params_t*
	),
	im_layout_params_t
);

/*
 * Simple horizontal, vertical (or both) stack that lays out subframes
 * in linear order.
 */
frame_t im_stack_layout_builder(
	const frame_t container,
	const frame_t frame,
	const insets_t insets,
	im_layout_params_t *params
);

void im_make_scrollable();

#endif
