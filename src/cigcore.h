#ifndef CIG_CORE_INCLUDED
#define CIG_CORE_INCLUDED

#include "ciglimit.h"
#include "cigmac.h"
#include "cigcorem.h"
#include "types/vec2.h"
#include "types/insets.h"
#include "types/frame.h"
#include "types/stack.h"
#include <stdbool.h>
#include <stddef.h>

/* ╔══════════════════════════╗
   ║ PUBLIC TYPE DECLARATIONS ║
   ╚══════════════════════════╝ */
	 
	 
/* These macros declare a foo<int> type essentially */
DECLARE_VEC2_T(int, cig_vec2, -999999) /* Declares `cig_vec2_t` */
DECLARE_INSETS_T(int, insets) /* Declares `insets_t` */
DECLARE_FRAME_T(int, frame, cig_vec2, insets) /* Declares `frame_t` */


/* All layout element get a unique ID that tries to be unique across frames, but no promises.
   See `im_next_id` how to definitely keep things consistent */
typedef unsigned long IMGUIID;


/* Opaque pointer to a buffer/screen/texture/etc to be renderered into */
typedef void* im_buffer_ref;


/* */
typedef enum {
	CASEFLAG(0, IM_CULL_SUBFRAMES),
	
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
	void *custom_data;
	
	/* (( PRIVATE )) */		
	int _horizontal_position, _vertical_position, _h_size, _v_size;
	struct {
		int h_cur, v_cur, total; /* h_ and v_cur are only counted in stacks/grids */
	} _count;
} im_layout_params_t;


/* */
typedef struct {
  cig_vec2_t offset;
	cig_vec2_t content_size;
} im_scroll_state_t;


/* */
typedef struct {
	IMGUIID id;
	frame_t frame; /* Relative frame */
	frame_t clipped_frame; /* Relative clipped frame */
	frame_t absolute_frame; /* Screen-space frame */
	insets_t insets; /* Insets affect child elements within this element */
	
	/* (( PRIVATE )) */		
	bool (*_layout_function)(frame_t, frame_t, im_layout_params_t*, frame_t*);
	im_layout_params_t _layout_params;
  bool _clipped, _interaction_enabled;
  im_scroll_state_t *_scroll_state;
	unsigned int _id_counter;
} im_element_t;


typedef enum {
	CASEFLAG(0, IM_MOUSE_BUTTON_LEFT),
	CASEFLAG(1, IM_MOUSE_BUTTON_RIGHT),
	IM_MOUSE_BUTTON_ANY = IM_MOUSE_BUTTON_LEFT | IM_MOUSE_BUTTON_RIGHT
} im_mouse_button_t;


typedef struct {
	im_mouse_button_t button_mask;
	im_mouse_button_t last_button_down;
	im_mouse_button_t last_button_up;
	cig_vec2_t position;
	enum {
		NEITHER,	/* Button was neither pressed or released this frame */
		BEGAN,		/* Button was pressed down this frame (click started) */
		ENDED,		/* Button was released this frame (click ended) */
		EXPIRED		/* Button was held longer than deemed appropriate */
	} click_state;
	struct {
		bool active;
		cig_vec2_t start_position;
		cig_vec2_t change;
	} drag;
	bool locked; /* Elements are not tracked. Set to TRUE by widgets that want exclusive
	                use of drag state. A scrollbar thumb for example where buttons and
                  other elements should not be highlighted even if hovered while moving */
	
	/* (( PRIVATE )) */								
	unsigned int _press_start_tick;
	unsigned int _target_prev_frame;
	unsigned int _target_this_frame;
	IMGUIID _press_target_id; /* Element that was focused when button press began */
} im_mouse_state_t;

typedef enum {
	/* `IM_MOUSE_PRESS_INSIDE` option specifies whether the press has to start
	within the bounds of this element. Otherwise it can start outside and the 
	element reflects pressed state as soon as mouse moves onto it */
	CASEFLAG(0, IM_MOUSE_PRESS_INSIDE)
} im_press_options_t;


typedef enum {
	CASEFLAG(0, IM_CLICK_STARTS_INSIDE),
	CASEFLAG(1, IM_CLICK_ON_BUTTON_DOWN),
	CASEFLAG(2, IM_CLICK_EXPIRE),
	IM_CLICK_DEFAULT_OPTIONS = IM_CLICK_STARTS_INSIDE
} im_click_options_t;


DECLARE_STACK(im_element_t);



/* ╔══════════════════════════════════════════════════╗
   ║                   PUBLIC API                     ║
   ╚══════════════════════════════════════════════════╝ */
	 
	 
	 
/* ┌───────────────┐
───┤  CORE LAYOUT  │
   └───────────────┘ */


/* */
void im_begin_layout(im_buffer_ref, frame_t);


/* */
void im_end_layout();


/* Resets internal values. Useful for when transitioning to a different
   game state or screen where you lay out a completely different UI */
void im_reset_internal_state();


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
	bool (*)(frame_t, frame_t, im_layout_params_t*, frame_t*),
	frame_t frame,
	insets_t insets,
	im_layout_params_t
);


/* Pop and return the last element in the layout stack */
im_element_t* im_pop_frame();


/* Returns current layout element */
im_element_t* im_element();


/* Returns current local frame relative to its parent */
frame_t im_relative_frame();


/* Returns current screen-space frame */
frame_t im_absolute_frame();


/* Converts a relative frame to a screen-space frame */
frame_t im_convert_relative_frame(frame_t);


/* Returns a pointer to the current layout element stack. Avoid accessing if possible. */
STACK(im_element_t)* im_element_stack();



/* ┌────────────────────────────────┐
───┤  TEMPORARY BUFFERS (ADVANCED)  │
   └────────────────────────────────┘ */
	 

/* Similar to `im_begin_layout` where you start rendering into a new buffer,
   in the current element's coordinate system. All subsequent elements `absolute_frame`-s
	 are relative to this buffer/screen/texture.
	 
	 This is mostly when you want to cache
   some more complex widget, like a large text view or similar. You can internally
   check whether you need to redraw or just re-render the old buffer/screen/texture */	 
void im_push_buffer(im_buffer_ref);


/* Pops the previously pushed buffer. Does not reset anything else about the state
   of the UI, unline `im_end_layout` */
void im_pop_buffer();



/* ┌─────────────────────┐
───┤  MOUSE INTERACTION  │
   └─────────────────────┘ */
	 

/* Pass mouse coordinates and button press state(s) */
void im_set_input_state(cig_vec2_t, im_mouse_button_t);


/* Returns the current mouse state as updated by last  `im_set_input_state` call */
im_mouse_state_t *im_mouse_state();


/* Enables mouse tracking for the current layout element.
   Call this after a successful `im_push_frame` call */
void im_enable_interaction();


/* Checks if the current layout element is the topmost element under the cursor */
bool im_hovered();


/* Checks if the current element is hovered and the mouse button is pressed.
   See `im_press_options_t` declaration for more info */
im_mouse_button_t im_pressed(im_mouse_button_t, im_press_options_t);


/* Checks if the current element is hovered and mouse button was clicked or released
   depending on the options. See `im_click_options_t` declaration for more info */
im_mouse_button_t im_clicked(im_mouse_button_t, im_click_options_t);



/* ┌─────────────┐
───┤  SCROLLING  │
   └─────────────┘ */


/* Tries to enable scrolling for the current layout element and allocate an
   internal scroll state that remains constant between frames. The state
	 is released when the element is no longer on screen.
	 
	 To enable to a proper persistent scroll state, you can provide a pointer to
	 the struct stored somewhere in your application layer. Pass NULL for
	 the default behavior described above.
	 
	 The state pool is limited to `IM_STATES_MAX` and if there are too many
	 scrolling elements already, it may fail.
	 
	 @return TRUE if state could be allocated, FALSE otherwise */
bool im_enable_scroll(im_scroll_state_t *);


/* Set scroll offset values */
void im_set_offset(cig_vec2_t);


/* Change scroll offset values */
void im_change_offset(cig_vec2_t);


/* Get scroll offset values */
cig_vec2_t im_offset();


/* Get scroll content size */
cig_vec2_t im_content_size();


/* Returns the current scroll state objet, NULL if scrolling is not enabled */
im_scroll_state_t* im_scroll_state();



/* ┌──────────────────┐
───┤  LAYOUT HELPERS  │
   └──────────────────┘ */


/* By default, when adding frames that are completely outside the bounds
   of the parent, `im_push_frame` calls return false. You can disable that
   behavior with this */
void im_disable_culling();


/* Enables clipping for the current layout element */
void im_enable_clipping();


/* Normally element ID is auto-calculated and may vary from frame to frame.
   This sets an explicit Id for the next `im_push_frame` call.
   See `im_hash` for generating an ID from a string */
void im_set_next_id(IMGUIID);


/* Returns the depth of the layout stack currently */
unsigned int im_depth();


/* Generates an ID from a string */
IMGUIID im_hash(const char *str);


/* Pushes and pops an empty frame to trigger a layout function to allocate space.
   Useful when you have a stack or grid and want to trigger a new line or column */
void im_empty();


/* */
void im_spacer(int size);


/* Default layout function for stack and grid type */
bool im_default_layout_builder(frame_t, frame_t, im_layout_params_t*, frame_t*);


#endif
