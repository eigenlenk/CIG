#ifndef IM_UMBRELLA_H
#define IM_UMBRELLA_H

#include "imlimits.h"
#include "imguim.h"
#include "stack.h"
#include "types/frame.h"
#include "types/insets.h"
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
#define IM_CENTERED(W, H) frame_make((IM_W * 0.5) - (W * 0.5), (IM_H * 0.5) - (H * 0.5), W, H)
#define IM_CENTER vec2_make((IM_W * 0.5), (IM_H * 0.5))
#define IM_R (IM_X + IM_W)
#define IM_B (IM_Y + IM_H)
#define CIM_SCROLL_LIMIT_X im_current_element()->_scroll_state->content_size.x-im_current_element()->frame.w+im_current_element()->insets.left+im_current_element()->insets.right
#define CIM_SCROLL_LIMIT_Y im_current_element()->_scroll_state->content_size.y-im_current_element()->frame.h+im_current_element()->insets.top+im_current_element()->insets.bottom

#define IM_INLINE inline __attribute__((always_inline))

/* ╔══════════════════════════════╗
   ║   PUBLIC TYPE DECLARATIONS   ║
   ╚══════════════════════════════╝ */


typedef unsigned long IMGUIID;


typedef void* im_buffer_ref;


typedef enum {
	BITFLAG(0, IM_CULL_SUBFRAMES),
	
	IM_DEFAULT_LAYOUT_FLAGS = IM_CULL_SUBFRAMES
} im_layout_options_t;


/* Structure containing parameters passed to layout function */
typedef struct {
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
	void *user_data;
	
	/* Private. Keep out! */
	int _horizontal_position, _vertical_position, _h_size, _v_size;
	struct {
		int h_cur, v_cur, total; /* h_ and v_cur are only counted in stacks/grids */
	} _count;
} im_layout_params_t;


typedef struct {
  vec2 offset;
	vec2 content_size;
} im_scroll_state_t;


typedef struct {
	IMGUIID id;
	frame_t frame;
	frame_t absolute_frame;
  insets_t insets;
	
	/* Private. Keep out! */
	bool (*_layout_function)(const frame_t, const frame_t, im_layout_params_t *, frame_t *);
	im_layout_params_t _layout_params;
  bool _clipped, _interaction_enabled;
  im_scroll_state_t *_scroll_state;
	unsigned int _id_counter;
} im_element_t;


DECLARE_STACK(im_element_t);


typedef enum {
	CASEFLAG(0, IM_MOUSE_BUTTON_LEFT),
	CASEFLAG(1, IM_MOUSE_BUTTON_RIGHT),
	IM_MOUSE_BUTTON_ANY = IM_MOUSE_BUTTON_LEFT | IM_MOUSE_BUTTON_RIGHT
} im_mouse_button_t;


typedef enum {
	CASEFLAG(0, IM_MOUSE_PRESS_INSIDE)
} im_press_options_t;


typedef enum {
	CASEFLAG(0, IM_CLICK_STARTS_INSIDE),
	CASEFLAG(1, IM_CLICK_ON_BUTTON_DOWN),
	CASEFLAG(2, IM_CLICK_EXPIRE)
} im_click_options_t;


/* ╔════════════════╗
   ║   PUBLIC API   ║
   ╚════════════════╝ */


/* (( LAYOUT FUNCTIONS )) */

/* */
void im_begin_layout(im_buffer_ref, frame_t);


/* */
void im_end_layout();


/* Pass mouse coordinates and button press state(s) */
void im_set_input_state(vec2, im_mouse_button_t);


/* Resets internal values. Useful for when transitioning to a different
   game state or screen where you lay out a completely different UI */
void imgui_reset_internal_state();


/* Returns an opaque pointer to the current buffer where drawing operations would take place */
im_buffer_ref im_buffer();


/* Pushes a new frame with zero insets to layout stack.
   @return TRUE if frame is visible within current container, FALSE otherwise */
bool im_push_frame(frame_t);


/* Push a new frame with custom insets to layout stack.
   @return TRUE if frame is visible within current container, FALSE otherwise */
bool im_push_frame_insets(frame_t frame, insets_t insets);


/* Push a new frame with custom insets and params to layout stack.
   @return TRUE if frame is visible within current container, FALSE otherwise */
bool im_push_frame_insets_params(frame_t frame, insets_t insets, im_layout_params_t);


/* Push layout builder function to layout stack.
   @return TRUE if frame is visible within current container, FALSE otherwise */
bool im_push_frame_function(
	frame_t frame,
	insets_t insets,
	im_layout_params_t,
	bool (*)(frame_t, frame_t, im_layout_params_t*, frame_t*)
);


/* Pop and return the last element in the layout stack */
im_element_t* im_pop_frame();


/* Returns current layout element */
im_element_t* im_element();


/* Returns current local frame relative to its parent */
IM_INLINE frame_t im_relative_frame();


/* Returns current screen-space frame */
IM_INLINE frame_t im_absolute_frame();


/* Returns root frame for the current buffer */
frame_t imgui_root_frame();


/* Converts a relative frame to a screen-space frame */
frame_t im_convert_relative_frame(frame_t);


/* Returns a pointer to the current layout element stack. Avoid if possible. */
STACK(im_element_t)* im_frame_stack();


/* (( LAYOUT HELPERS )) */

/* Normally element ID is auto-calculated and may vary from frame to frame.
   This sets an explicit Id for the next `im_push_*` call.
   See: `im_hash` for generating an ID from a string */
void im_next_id(IMGUIID);

/* Returns the depth of the layout stack currently */
unsigned int im_depth();

/* Generates an ID from a string */
IMGUIID im_hash(const char *str);

/* Default layout function for stack and grid type */
bool im_default_layout_builder(frame_t, frame_t, im_layout_params_t*, frame_t*);

/* Pushes and pops an empty frame to trigger a layout function to allocate space.
   Useful when you have a stack or grid and want to trigger a new line or column */
IM_INLINE void im_empty();


/* */
IM_INLINE void im_insert_spacer(const int size);



/* (( MOUSE INTERACTION )) */

/* Enables mouse tracking for the current layout element.
   Call this after a successful `im_push_*` call */
void im_enable_interaction();

/* Checks if the current layout element is the topmost element under the cursor */
bool im_hovered();

im_mouse_button_t im_pressed(const im_mouse_button_t, const im_press_options_t);

im_mouse_button_t im_clicked(const im_mouse_button_t, const im_click_options_t);










IM_INLINE im_scroll_state_t* im_scroll_state() { return im_current_element()->_scroll_state; }

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
