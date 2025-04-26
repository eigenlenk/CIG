#ifndef CIG_CORE_INCLUDED
#define CIG_CORE_INCLUDED

#include "ciglimit.h"
#include "cigmac.h"
#include "types/vec2.h"
#include "types/insets.h"
#include "types/rect.h"
#include "types/stack.h"
#include <stdbool.h>
#include <stddef.h>

/* ╔══════════════════════════╗
   ║ PUBLIC TYPE DECLARATIONS ║
   ╚══════════════════════════╝ */

/* These macros declare a templated type essentially */
DECLARE_VEC2_T   (int, cig_vec2, -999999) /* Declares `cig_vec2_t` */
DECLARE_INSETS_T (int, cig_insets) /* Declares `cig_cig_insets_t` */
DECLARE_RECT_T   (int, cig_rect, cig_vec2, cig_insets) /* Declares `cig_rect_t` */

/* All layout element get a unique ID that tries to be unique across frames, but no promises.
   See `cig_next_id` how to definitely keep things consistent */
typedef unsigned long cig_id_t;

/* Opaque pointer to a buffer/screen/texture/etc to be renderered into */
typedef void* cig_buffer_ref;

/* Structure containing parameters passed to layout function */
typedef struct {
  /* One or more axis which a builder uses to position children */
  enum {
    CIG__NOAXIS = 0,
    CIG_LAYOUT_AXIS_HORIZONTAL = CIG_BIT(1),
    CIG_LAYOUT_AXIS_VERTICAL = CIG_BIT(2),
    CIG_LAYOUT_AXIS_BOTH = CIG_LAYOUT_AXIS_HORIZONTAL | CIG_LAYOUT_AXIS_VERTICAL
  } axis;

  /* Direction in which the layout flows. Used by default grid builder */
  enum {
    CIG_LAYOUT_DIRECTION_HORIZONTAL = 0,
    CIG_LAYOUT_DIRECTION_VERTICAL
  } direction;

  struct {
    enum {
      CIG_LAYOUT_ALIGNS_LEFT = 0,
      CIG_LAYOUT_ALIGNS_RIGHT
    } horizontal;
    enum {
      CIG_LAYOUT_ALIGNS_TOP = 0,
      CIG_LAYOUT_ALIGNS_BOTTOM
    } vertical;
  } alignment;

  /* Some common parameters the layout builder could use */
  int spacing, width, height, columns, rows;

  /* Limits how many elements can be added per axis on in total.
     Total number of elements is checked in `cig_push_frame` but horizontal
     and vertical limits are only used in the default stack/grid builder */
	struct {
    int horizontal, vertical, total;
  } limit;
  
  /* Some basic layout flags */
	enum {
    CIG_LAYOUT_DISABLE_CULLING = CIG_BIT(0)
  } flags;
  
  /* Opaque pointer for passing custom data to a custom layout builder */
	void *custom_data;
	
	/* PRIVATE: */		
	int _h_pos, _v_pos, _h_size, _v_size;
	struct {
    int h_cur, v_cur, total;
  } _count; /* h_ and v_cur are only counted in stacks/grids */
} cig_layout_params_t;

typedef struct {
  unsigned char bytes[CIG_STATE_MEM_ARENA_BYTES];
  size_t mapped;
} cig_state_memory_arena_t;

typedef struct {
	enum {
		INACTIVE = 0,
		ACTIVATED,
		ACTIVE
	} activation_state;
	cig_state_memory_arena_t arena;
} cig_state_t;

/* */
typedef struct {
  cig_vec2_t offset;
	cig_vec2_t content_size;
} cig_scroll_state_t;

typedef struct {
  cig_rect_t rect;
  cig_insets_t insets;
  cig_layout_params_t params;
  bool (*builder)(cig_rect_t, cig_rect_t, cig_layout_params_t*, cig_rect_t*);
} cig_frame_args_t;

/* */
typedef struct {
	cig_id_t id;
	cig_rect_t rect; /* Relative rect */
	cig_rect_t clipped_rect; /* Relative clipped rect */
	cig_rect_t absolute_rect; /* Screen-space rect */
	cig_insets_t insets; /* Insets affect child elements within this element */
	
	/* PRIVATE: */		
	bool (*_layout_function)(cig_rect_t, cig_rect_t, cig_layout_params_t*, cig_rect_t*);
	cig_layout_params_t _layout_params;
  bool _clipped, _interaction_enabled;
  cig_scroll_state_t *_scroll_state;
  cig_state_t *_state;
	unsigned int _id_counter;
} cig_frame_t;

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
		NEITHER,	/* Button was neither pressed or released */
		BEGAN,		/* Button was pressed down (click started) */
		ENDED,		/* Button was released this (click ended) */
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
	
	/* PRIVATE: */								
	unsigned int _press_start_tick;
	cig_id_t _press_target_id, /* Element that was focused when button press began */
           _target_prev_tick,
           _target_this_tick;
} cig_input_state_t;

typedef enum {
	/* `CIG_PRESS_INSIDE` option specifies whether the press has to start
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

typedef cig_rect_t cig_clip_rect_t;
#define STACK_CAPACITY_cig_clip_rect_t CIG_BUFFER_CLIP_REGIONS_MAX
DECLARE_ARRAY_STACK_T(cig_clip_rect_t);

typedef struct {
	cig_buffer_ref buffer;
	cig_vec2_t origin;
	cig_clip_rect_t_stack_t clip_rects;
} cig_buffer_element_t;

#define STACK_CAPACITY_cig_frame_t CIG_NESTED_ELEMENTS_MAX
DECLARE_ARRAY_STACK_T(cig_frame_t);

#define STACK_CAPACITY_cig_buffer_element_t CIG_BUFFERS_MAX
DECLARE_ARRAY_STACK_T(cig_buffer_element_t);

/* A single instance of CIG. Use one for each game state?
   Should be considered an opaque type! */
typedef struct {
  /* PRIVATE: */	
  cig_frame_t_stack_t frames;
  cig_buffer_element_t_stack_t buffers;
  cig_input_state_t input_state;
  cig_insets_t default_insets;
  cig_id_t next_id;
  unsigned int tick;
  struct {
    cig_id_t id;
    unsigned int last_tick;
    cig_scroll_state_t value;
  } scroll_elements[CIG_SCROLLABLE_ELEMENTS_MAX];
  struct {
    cig_id_t id;
    cig_state_t value;
    unsigned int last_tick;
  } state_list[CIG_STATES_MAX];
} cig_context_t;

/* ╔══════════════════════════════════════════════════╗
   ║                   PUBLIC API                     ║
   ╚══════════════════════════════════════════════════╝ */

/* ┌───────────────┐
───┤  CORE LAYOUT  │
   └───────────────┘ */
   
/* Call this once to initalize (or reset) the context */
void cig_init_context(cig_context_t*);

/* */
void cig_begin_layout(cig_context_t*, CIG_OPTIONAL(cig_buffer_ref), cig_rect_t);

/* */
void cig_end_layout();

/* Returns an opaque pointer to the current buffer where drawing operations would take place */
cig_buffer_ref cig_buffer();

/* Pushes a new frame with args struct containing all relevant data.
   @return TRUE if rect is visible within current container, FALSE otherwise */
bool cig_push_frame_args(cig_frame_args_t);

/* Pushes a new frame with default insets (see `cig_set_default_insets`) to layout stack.
   @return TRUE if rect is visible within current container, FALSE otherwise */
bool cig_push_frame(cig_rect_t);

/* Push a new frame with custom insets to layout stack.
   @return TRUE if rect is visible within current container, FALSE otherwise */
bool cig_push_frame_insets(cig_rect_t, cig_insets_t);

/* Push a new frame with custom insets and params to layout stack.
   @return TRUE if rect is visible within current container, FALSE otherwise */
bool cig_push_frame_insets_params(cig_rect_t, cig_insets_t, cig_layout_params_t);

/* Push layout builder function to layout stack.
   @return TRUE if rect is visible within current container, FALSE otherwise */
bool cig_push_layout_function(
	bool (*)(cig_rect_t, cig_rect_t, cig_layout_params_t*, cig_rect_t*),
	cig_rect_t,
	cig_insets_t,
	cig_layout_params_t
);

/* Pop and return the last element in the layout stack */
cig_frame_t* cig_pop_frame();

/* Sets insets used by all consecutive `cig_push_frame` calls */
void cig_set_default_insets(cig_insets_t);

/* Returns current layout element */
cig_frame_t* cig_frame();

/* Returns current local rect relative to its parent */
CIG_INLINED cig_rect_t cig_rect() { return cig_frame()->rect; }

/* Returns current local rect that's been clipped */
CIG_INLINED cig_rect_t cig_clipped_rect() { return cig_frame()->clipped_rect; }

/* Returns current screen-space rect */
CIG_INLINED cig_rect_t cig_absolute_rect() { return cig_frame()->absolute_rect; }

/* Converts a relative rect to a screen-space rect */
cig_rect_t cig_convert_relative_rect(cig_rect_t);

/* Returns a pointer to the current layout element stack. Avoid accessing if possible. */
cig_frame_t_stack_t* cig_frame_stack();

/* ┌─────────┐
───┤  STATE  │
   └─────────┘ */
   
CIG_OPTIONAL(cig_state_t*) cig_state();

CIG_OPTIONAL(void*) cig_state_allocate(size_t);

/* ┌────────────────────────────────┐
───┤  TEMPORARY BUFFERS (ADVANCED)  │
   └────────────────────────────────┘ */

/* Similar to `cig_begin_layout` where you start rendering into a new buffer,
   in the current element's coordinate system. All subsequent elements `absolute_rect`-s
	 are relative to this buffer/screen/texture.
	 
	 This is mostly when you want to cache
   some more complex widget, like a large text view or similar. You can internally
   check whether you need to redraw or just re-render the old buffer/screen/texture */	 
void cig_push_buffer(cig_buffer_ref);

/* Pops the previously pushed buffer. Does not reset anything else about the state
   of the UI, unlike `cig_end_layout` */
void cig_pop_buffer();

/* ┌─────────────────────┐
───┤  MOUSE INTERACTION  │
   └─────────────────────┘ */

/* Pass mouse coordinates and button press state[s] */
void cig_set_input_state(cig_vec2_t, cig_input_action_type_t);

/* Returns the current mouse state as updated by last `cig_set_input_state` call */
CIG_OPTIONAL(cig_input_state_t*) cig_input_state();

/* Enables mouse tracking for the current layout element.
   Call this after a successful `cig_push_frame` call */
void cig_enable_interaction();

/* Checks if the current layout element is the topmost element under the cursor */
bool cig_hovered();

/* Checks if the current element is hovered and the mouse button is pressed.
   See `cig_press_flags_t` declaration for more info */
cig_input_action_type_t cig_pressed(cig_input_action_type_t, cig_press_flags_t);

/* Checks if the current element is hovered and mouse button was clicked or released
   depending on the options. See `cig_click_flags_t` declaration for more info */
cig_input_action_type_t cig_clicked(cig_input_action_type_t, cig_click_flags_t);

/* ┌─────────────┐
───┤  SCROLLING  │
   └─────────────┘ */
   
/* Attempts to enable scrolling for the current layout element and allocate an
   internal scroll state that remains constant between ticks. You can also provide
   a pointer to the struct stored somewhere in your application layer. Pass NULL for
	 the default behavior described above.
	 
	 The state pool is limited to `CIG_STATES_MAX` and if there are too many
	 scrolling elements already, it may fail.
	 
	 @return TRUE if state could be allocated, FALSE otherwise */
bool cig_enable_scroll(cig_scroll_state_t *);

/* Returns the current scroll state objet, NULL if scrolling is not enabled */
CIG_OPTIONAL(cig_scroll_state_t*) cig_scroll_state();

/* Set scroll offset values */
void cig_set_offset(cig_vec2_t);

/* Change scroll offset values */
void cig_change_offset(cig_vec2_t);

/* Get scroll offset values */
cig_vec2_t cig_offset();

/* Get scroll content size */
cig_vec2_t cig_content_size();

/* ┌──────────────────┐
───┤  LAYOUT HELPERS  │
   └──────────────────┘ */

/* By default, when adding rects that are completely outside the bounds
   of the parent, `cig_push_frame` calls return FALSE. You can disable that
   behavior with this */
void cig_disable_culling();

/* Enables clipping for the current layout element */
void cig_enable_clipping();

/* Normally element ID is auto-calculated and may vary from tick to tick.
   This sets an explicit Id for the next `cig_push_frame` call.
   See `cig_hash` for generating an ID from a string */
void cig_set_next_id(cig_id_t);

/* Returns the depth of the layout stack currently */
unsigned int cig_depth();

/* Generates an ID from a string */
cig_id_t cig_hash(const char *str);

/* Pushes and pops an empty frame to trigger a layout function to allocate space.
   Useful when you have a stack or grid and want to trigger a new line or column */
void cig_empty();

/* */
void cig_spacer(int size);

/* Default layout function for stack and grid type */
bool cig_default_layout_builder(cig_rect_t, cig_rect_t, cig_layout_params_t*, cig_rect_t*);

#endif
