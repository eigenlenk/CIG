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


/* These macros declare a templated type essentially */
DECLARE_VEC2_T    (int, cig_vec2, -999999) /* Declares `cig_vec2_t` */
DECLARE_insets_t  (int, cig_insets) /* Declares `cig_cig_insets_t` */
DECLARE_frame_t   (int, cig_frame, cig_vec2, cig_insets) /* Declares `cig_cig_frame_t` */


/* All layout element get a unique ID that tries to be unique across frames, but no promises.
   See `im_next_id` how to definitely keep things consistent */
typedef unsigned long cig_id_t;


/* Opaque pointer to a buffer/screen/texture/etc to be renderered into */
typedef void* im_buffer_ref;


/* */
typedef enum {
	CIG_CULL_SUBFRAMES = CIG_BIT(0),
	CIG_DEFAULT_LAYOUT_FLAGS = CIG_CULL_SUBFRAMES
} cig_layout_flags_t;


/* Structure containing parameters passed to layout function */
typedef struct {
  /* One or more axis which a builder uses to position children */
  enum {
    CIG__NOAXIS = 0,
    CIG_LAYOUT_AXIS_HORIZONTAL = CIG_BIT(1),
    CIG_LAYOUT_AXIS_VERTICAL = CIG_BIT(2)
  } axis;

  /* Direction in which the layout flows. Used by default grid builder */
  enum {
    CIG_LAYOUT_DIRECTION_LEFT = 0,
    CIG_LAYOUT_DIRECTION_DOWN
  } direction;

  /* Some common parameters the layout builder could use */
  int spacing,
      width,
      height,
      columns,
      rows;

  /* Limits how many elements can be added per axis on in total.
     Total number of elements is checked in `im_push_frame` but horizontal
     and vertical limits are only used in the default stack/grid builder */
	struct { int horizontal, vertical, total; } limit;
  
  /* Some basic layout flags */
	cig_layout_flags_t flags;
  
  /* Opaque pointer for passing custom data to a custom layout builder */
	void *custom_data;
	
	/* (( PRIVATE )) */		
	int _horizontal_position,
      _vertical_position,
      _h_size,
      _v_size;
	struct {
    int h_cur,
        v_cur,
        total;
  } _count; /* h_ and v_cur are only counted in stacks/grids */
} cig_layout_params_t;


/* */
typedef struct {
  cig_vec2_t offset;
	cig_vec2_t content_size;
} cig_scroll_state_t;


/* */
typedef struct {
	cig_id_t id;
	cig_frame_t frame; /* Relative frame */
	cig_frame_t clipped_frame; /* Relative clipped frame */
	cig_frame_t absolute_frame; /* Screen-space frame */
	cig_insets_t insets; /* Insets affect child elements within this element */
	
	/* (( PRIVATE )) */		
	bool (*_layout_function)(cig_frame_t, cig_frame_t, cig_layout_params_t*, cig_frame_t*);
	cig_layout_params_t _layout_params;
  bool _clipped, _interaction_enabled;
  cig_scroll_state_t *_scroll_state;
	unsigned int _id_counter;
} cig_element_t;


typedef enum {
  CIG_INPUT_MOUSE_BUTTON_LEFT = CIG_BIT(0),
  CIG_INPUT_MOUSE_BUTTON_RIGHT = CIG_BIT(1),
	CIG_MOUSE_BUTTON_ANY = CIG_INPUT_MOUSE_BUTTON_LEFT | CIG_INPUT_MOUSE_BUTTON_RIGHT
} cig_input_action_type_t;


typedef struct {
	cig_input_action_type_t action_mask;
	cig_input_action_type_t last_action_began;
	cig_input_action_type_t last_action_ended;
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
	cig_id_t _press_target_id, /* Element that was focused when button press began */
           _target_prev_frame,
           _target_this_frame;
} im_input_state_t;


typedef enum {
	/* `IM_PRESS_INSIDE` option specifies whether the press has to start
	within the bounds of this element. Otherwise it can start outside and the 
	element reflects pressed state as soon as mouse moves onto it */
  CIG_PRESS_INSIDE = CIG_BIT(0)
} cig_press_flags_t;


typedef enum {
  CIG_CLICK_STARTS_INSIDE = CIG_BIT(0),
  CIG_CLICK_ON_PRESS = CIG_BIT(1),
  CIG_CLICK_EXPIRE = CIG_BIT(2),
	CIG_CLICK_DEFAULT_OPTIONS = CIG_CLICK_STARTS_INSIDE
} cig_click_flags_t;


#define STACK_CAPACITY_cig_element_t CIG_ELEMENTS_MAX
DECLARE_ARRAY_STACK_T(cig_element_t);



/* ╔══════════════════════════════════════════════════╗
   ║                   PUBLIC API                     ║
   ╚══════════════════════════════════════════════════╝ */
	 
	 
	 
/* ┌───────────────┐
───┤  CORE LAYOUT  │
   └───────────────┘ */


/* */
void im_begin_layout(im_buffer_ref, cig_frame_t);


/* */
void im_end_layout();


/* Resets internal values. Useful for when transitioning to a different
   game state or screen where you lay out a completely different UI */
void im_reset_internal_state();


/* Returns an opaque pointer to the current buffer where drawing operations would take place */
im_buffer_ref im_buffer();


/* Pushes a new frame with zero insets to layout stack.
   @return TRUE if frame is visible within current container, FALSE otherwise */
bool im_push_frame(cig_frame_t);


/* Push a new frame with custom insets to layout stack.
   @return TRUE if frame is visible within current container, FALSE otherwise */
bool im_push_frame_insets(cig_frame_t frame, cig_insets_t insets);


/* Push a new frame with custom insets and params to layout stack.
   @return TRUE if frame is visible within current container, FALSE otherwise */
bool im_push_frame_insets_params(cig_frame_t frame, cig_insets_t insets, cig_layout_params_t);


/* Push layout builder function to layout stack.
   @return TRUE if frame is visible within current container, FALSE otherwise */
bool im_push_frame_function(
	bool (*)(cig_frame_t, cig_frame_t, cig_layout_params_t*, cig_frame_t*),
	cig_frame_t frame,
	cig_insets_t insets,
	cig_layout_params_t
);


/* Pop and return the last element in the layout stack */
cig_element_t* im_pop_frame();


/* Returns current layout element */
cig_element_t* im_element();


/* Returns current local frame relative to its parent */
CIG_INLINED cig_frame_t im_frame() { return im_element()->frame; }


/* Returns current local frame that's been clipped */
CIG_INLINED cig_frame_t im_clipped_frame() { return im_element()->clipped_frame; }


/* Returns current screen-space frame */
CIG_INLINED cig_frame_t im_absolute_frame() { return im_element()->absolute_frame; }


/* Converts a relative frame to a screen-space frame */
cig_frame_t im_convert_relative_frame(cig_frame_t);


/* Returns a pointer to the current layout element stack. Avoid accessing if possible. */
stack_cig_element_t_t* im_element_stack();



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
	 

/* Pass mouse coordinates and button press state[s] */
void im_set_input_state(cig_vec2_t, cig_input_action_type_t);


/* Returns the current mouse state as updated by last  `im_set_input_state` call */
im_input_state_t *im_input_state();


/* Enables mouse tracking for the current layout element.
   Call this after a successful `im_push_frame` call */
void im_enable_interaction();


/* Checks if the current layout element is the topmost element under the cursor */
bool im_hovered();


/* Checks if the current element is hovered and the mouse button is pressed.
   See `im_press_flags_t` declaration for more info */
cig_input_action_type_t im_pressed(cig_input_action_type_t, cig_press_flags_t);


/* Checks if the current element is hovered and mouse button was clicked or released
   depending on the options. See `im_click_flags_t` declaration for more info */
cig_input_action_type_t im_clicked(cig_input_action_type_t, cig_click_flags_t);



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
bool im_enable_scroll(cig_scroll_state_t *);


/* Set scroll offset values */
void im_set_offset(cig_vec2_t);


/* Change scroll offset values */
void im_change_offset(cig_vec2_t);


/* Get scroll offset values */
cig_vec2_t im_offset();


/* Get scroll content size */
cig_vec2_t im_content_size();


/* Returns the current scroll state objet, NULL if scrolling is not enabled */
cig_scroll_state_t* im_scroll_state();



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
void im_set_next_id(cig_id_t);


/* Returns the depth of the layout stack currently */
unsigned int im_depth();


/* Generates an ID from a string */
cig_id_t im_hash(const char *str);


/* Pushes and pops an empty frame to trigger a layout function to allocate space.
   Useful when you have a stack or grid and want to trigger a new line or column */
void im_empty();


/* */
void im_spacer(int size);


/* Default layout function for stack and grid type */
bool im_default_layout_builder(cig_frame_t, cig_frame_t, cig_layout_params_t*, cig_frame_t*);


#endif
