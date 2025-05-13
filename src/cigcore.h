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
#include <stdint.h>

/*  ┌──────────────────────────┐
    │ PUBLIC TYPE DECLARATIONS │
    └──────────────────────────┘ */

/*  These macros declare a templated type essentially */
DECLARE_VEC2_T(int32_t, cig_v, -999999)
DECLARE_INSETS_T(int32_t, cig_i)
DECLARE_RECT_T(int32_t, cig_r, cig_v, cig_i)

/*  A couple of option bits we can use with rect components */
#define CIG__AUTO_BIT CIG_BIT(30)
#define CIG__REL_BIT CIG_BIT(29)

/*  Indicates the size of this element will be auto-calculated.
    When used in standard layout frames, AUTO essentially means to fill parent.
    In stack and grid builders it's more flexible and mostly follows parent's layout rules.
    You can pass in an additional value using CIG_REL to modify the final size.

    Eg. CIG_AUTO(CIG_REL(2)) to get a rectangle that's double the size */
#define CIG_AUTO(...) CIG__AUTO_X(,##__VA_ARGS__, CIG__AUTO_1(__VA_ARGS__), CIG__AUTO_0(__VA_ARGS__))

#define CIG__AUTO_0() CIG__AUTO_BIT
#define CIG__AUTO_1(VALUE) (VALUE < 0 ? (VALUE & ~CIG__AUTO_BIT) : (CIG__AUTO_BIT | VALUE))
#define CIG__AUTO_X(__X__,A,FUNC,...) FUNC

/*  Allows setting a relative position or size. Always relative to parent size,
    or in case of stacks and grids, the size available to the next element. */
#define CIG_REL(PERCENTAGE) (PERCENTAGE < 0 ? ((int)(PERCENTAGE * 100000) & ~CIG__REL_BIT) : (CIG__REL_BIT | (int)(PERCENTAGE * 100000)))

/*  Full AUTO rect */
#define RECT_AUTO cig_r_make(0, 0, CIG_AUTO(), CIG_AUTO())

/*  Full AUTO rect, but with overridable width or height */
#define RECT_AUTO_W(W) cig_r_make(0, 0, W, CIG_AUTO())
#define RECT_AUTO_H(H) cig_r_make(0, 0, CIG_AUTO(), H)

/*  Standard rectangle helpers */
#define RECT(X, Y, W, H) cig_r_make(X, Y, W, H)
#define RECT_SIZED(W, H) cig_r_make(0, 0, W, H)

/*  All-relative rectangle. All values should be in the range [0.0 ... 1.0] */
#define RECT_REL(X, Y, W, H) cig_r_make(CIG_REL(X), CIG_REL(Y), CIG_REL(W), CIG_REL(H))

#define CIG_FILL CIG_REL(1.0)
#define RECT_FILL cig_r_make(0, 0, CIG_FILL, CIG_FILL)

#define NO_INSETS cig_i_zero()

#define CIG_CLICK_EXPIRE_IN_SECONDS 0.5f

/*  All layout element get a unique ID that tries to be unique across frames, but no promises.
    See `cig_next_id` how to definitely keep things consistent */
typedef unsigned long cig_id_t;

/*  Opaque pointer to a buffer/screen/texture/etc to be renderered into */
typedef void* cig_buffer_ref;

typedef void (*cig_set_clip_rect_callback_t)(cig_buffer_ref, cig_r, bool);

/*  Structure containing parameters passed to layout function */
typedef struct {
  /*  One or more axis which a builder uses to position children */
  enum CIG_PACKED {
    CIG__NOAXIS = 0,
    CIG_LAYOUT_AXIS_HORIZONTAL = CIG_BIT(1),
    CIG_LAYOUT_AXIS_VERTICAL = CIG_BIT(2),
    CIG_LAYOUT_AXIS_BOTH = CIG_LAYOUT_AXIS_HORIZONTAL | CIG_LAYOUT_AXIS_VERTICAL
  } axis;

  /*  Direction in which the layout flows. Used by default grid builder */
  enum CIG_PACKED {
    CIG_LAYOUT_DIRECTION_HORIZONTAL = 0,
    CIG_LAYOUT_DIRECTION_VERTICAL
  } direction;

  struct {
    enum CIG_PACKED {
      CIG_LAYOUT_ALIGNS_LEFT = 0,
      CIG_LAYOUT_ALIGNS_CENTER,
      CIG_LAYOUT_ALIGNS_RIGHT
    } horizontal;
    enum CIG_PACKED {
      CIG_LAYOUT_ALIGNS_TOP = 0,
      CIG_LAYOUT_ALIGNS_MIDDLE,
      CIG_LAYOUT_ALIGNS_BOTTOM
    } vertical;
  } alignment;

  /*  Some common parameters the layout builder could use */
  int16_t spacing, width, height, columns, rows;

  /*  Limits how many elements can be added per axis on in total.
      Total number of elements is checked in `cig_push_frame` but horizontal
      and vertical limits are only used in the default stack/grid builder */
  struct {
    int16_t horizontal, vertical, total;
  } limit;

  struct {
    int32_t width, height;
  } size_max, size_min;

  /*  Some basic layout flags */
  enum CIG_PACKED {
    CIG_LAYOUT_DISABLE_CULLING = CIG_BIT(0)
  } flags;

  /*  Opaque pointer for passing custom data to a custom layout builder */
  void *custom_data;

  /*__PRIVATE__*/
  int32_t _h_pos, _v_pos, _h_size, _v_size;
  struct {
    int16_t h_cur, v_cur, total;
  } _count; /* h_ and v_cur are only counted in stacks/grids */
} cig_layout_params_t;

typedef struct {
  unsigned char bytes[CIG_STATE_MEM_ARENA_BYTES];
  size_t mapped, read;
} cig_state_memory_arena_t;

typedef struct {
  enum CIG_PACKED {
    INACTIVE = 0,
    ACTIVATED,
    ACTIVE
  } activation_state;
  cig_state_memory_arena_t arena;
} cig_state_t;

/* */
typedef struct {
  cig_v offset;
} cig_scroll_state_t;

typedef struct {
  cig_r rect;
  cig_i insets;
  cig_layout_params_t params;
  bool (*builder)(cig_r, cig_r, cig_layout_params_t*, cig_r*);
} cig_frame_args_t;

/* */
typedef struct {
  cig_id_t id;
  cig_r rect,          /* Relative rect */
        clipped_rect,  /* Relative clipped rect */
        absolute_rect, /* Screen-space rect */
        content_rect;  /* Relative rect bounding the content */
  cig_i insets;        /* Insets affect child elements within this element */

  /*__PRIVATE__*/      
  bool (*_layout_function)(cig_r, cig_r, cig_layout_params_t*, cig_r*);
  cig_scroll_state_t *_scroll_state;
  cig_state_t *_state;
  cig_layout_params_t _layout_params;
  unsigned int _id_counter;
  bool _clipped, _interaction_enabled;
} cig_frame_t;

typedef enum CIG_PACKED {
  CIG_INPUT_PRIMARY_ACTION = CIG_BIT(0),
  CIG_INPUT_SECONDARY_ACTION = CIG_BIT(1),
  CIG_INPUT_ACTION_ANY = CIG_INPUT_PRIMARY_ACTION | CIG_INPUT_SECONDARY_ACTION
} cig_input_action_type_t;

typedef struct {
  cig_input_action_type_t action_mask,
                          last_action_began,
                          last_action_ended;
  cig_v position;

  enum CIG_PACKED {
    NEITHER,  /* Button was neither pressed or released */
    BEGAN,    /* Button was pressed down (click started) */
    ENDED,    /* Button was released this (click ended) */
    EXPIRED   /* Button was held longer than deemed appropriate */
  } click_state;

  struct {
    bool active;
    cig_v start_position;
    cig_v change;
  } drag;

  /*  Elements are not tracked. Set to TRUE by widgets that want exclusive
      use of drag state. A scrollbar thumb for example where buttons and
      other elements should not be highlighted even if hovered while moving.
      reset to FALSE when drag ends */
  bool locked;

  /*__PRIVATE__*/
  float _press_start_time,
        _click_end_time;
  unsigned int _click_count;
  cig_id_t  _press_target_id,   /* Element that was focused when button press began */
            _target_prev_tick,
            _target_this_tick,
            _focus_target_this,
            _focus_target;
} cig_input_state_t;

typedef enum CIG_PACKED {
  /*  `CIG_PRESS_INSIDE` option specifies whether the press has to start
      within the bounds of this element. Otherwise it can start outside,
      and the element will reflect pressed state as soon as mouse moves onto it */
  CIG_PRESS_INSIDE = CIG_BIT(0),
  CIG_PRESS_DEFAULT_OPTIONS = CIG_PRESS_INSIDE
} cig_press_flags_t;

typedef enum CIG_PACKED {
  CIG_CLICK_STARTS_INSIDE = CIG_BIT(0),
  CIG_CLICK_ON_PRESS = CIG_BIT(1),
  CIG_CLICK_EXPIRE = CIG_BIT(2),
  CIG_CLICK_DOUBLE = CIG_BIT(3),
  CIG_CLICK_DEFAULT_OPTIONS = CIG_CLICK_STARTS_INSIDE
} cig_click_flags_t;

typedef cig_r cig_clip_rect_t;
#define STACK_CAPACITY_cig_clip_rect_t CIG_BUFFER_CLIP_REGIONS_MAX
DECLARE_ARRAY_STACK_T(cig_clip_rect_t);

typedef struct {
  cig_buffer_ref buffer;
  cig_v origin;
  cig_clip_rect_t_stack_t clip_rects;
} cig_buffer_element_t;

typedef cig_frame_t* cig_frame_ref;
#define STACK_CAPACITY_cig_frame_ref CIG_NESTED_ELEMENTS_MAX
DECLARE_ARRAY_STACK_T(cig_frame_ref);

#define STACK_CAPACITY_cig_buffer_element_t CIG_BUFFERS_MAX
DECLARE_ARRAY_STACK_T(cig_buffer_element_t);

/*  A single instance of CIG. Use one for each game state?
    Should be considered an opaque type! */
typedef struct {
  /*  __PRIVATE__ */
  cig_frame_ref_stack_t frame_stack;
  cig_buffer_element_t_stack_t buffers;
  cig_input_state_t input_state;
  cig_i default_insets;
  cig_id_t next_id;
  float delta_time,
        elapsed_time;
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
  struct {
    cig_frame_t elements[CIG_ELEMENTS_MAX];
    size_t count;
  } frames;
#ifdef DEBUG
  bool step_mode;
#endif
} cig_context_t;

/*  ┌─────────────┐
    │ CORE LAYOUT │
    └─────────────┘ */

/*  Call this once to initalize (or reset) the context */
void cig_init_context(cig_context_t*);

/* */
void cig_begin_layout(cig_context_t*, CIG_OPTIONAL(cig_buffer_ref), cig_r, float);

/* */
void cig_end_layout();

/*  Returns an opaque pointer to the current buffer where drawing operations would take place */
cig_buffer_ref cig_buffer();

/*  Pushes a new frame with args struct containing all relevant data.
    @return Reference to new element if rect is visible within current container, NULL otherwise */
cig_frame_t* cig_push_frame_args(cig_frame_args_t);

/*  Pushes a new frame with default insets (see `cig_set_default_insets`) to layout stack.
    @return Reference to new element if rect is visible within current container, NULL otherwise */
cig_frame_t* cig_push_frame(cig_r);

/*  Push a new frame with custom insets to layout stack.
    @return Reference to new element if rect is visible within current container, NULL otherwise */
cig_frame_t* cig_push_frame_insets(cig_r, cig_i);

/*  Push a new frame with custom insets and params to layout stack.
    @return Reference to new element if rect is visible within current container, NULL otherwise */
cig_frame_t* cig_push_frame_insets_params(cig_r, cig_i, cig_layout_params_t);

/*  Push layout builder function to layout stack.
    @return Reference to new element if rect is visible within current container, NULL otherwise */
cig_frame_t* cig_push_layout_function(
  bool (*)(cig_r, cig_r, cig_layout_params_t*, cig_r*),
  cig_r,
  cig_i,
  cig_layout_params_t
);

/*  Pop and return the last element in the layout stack */
cig_frame_t* cig_pop_frame();

/*  Sets insets used by all consecutive `cig_push_frame` calls */
void cig_set_default_insets(cig_i);

/*  @return Current layout element */
cig_frame_t* cig_frame();

/*  @return Current local rect relative to its parent */
CIG_INLINED cig_r cig_rect() { return cig_frame()->rect; }

/*  @return Current local rect that's been clipped */
CIG_INLINED cig_r cig_clipped_rect() { return cig_frame()->clipped_rect; }

/*  @return Current screen-space rect */
CIG_INLINED cig_r cig_absolute_rect() { return cig_frame()->absolute_rect; }

/*  @return Relative bounding rect for current content */
CIG_INLINED cig_r cig_content_rect() { return cig_frame()->content_rect; }

/*  Converts a relative rect to a screen-space rect */
cig_r cig_convert_relative_rect(cig_r);

/*  @return Pointer to the current layout element stack. Avoid accessing if possible. */
cig_frame_ref_stack_t* cig_frame_stack();

/*  ┌─────────────────────────────┐
    │ STATE & MEMORY ARENA ACCESS │
    └─────────────────────────────┘ */

CIG_OPTIONAL(cig_state_t*) cig_state();

/*  Allocates N bytes in current element's memory arena.
    @return Pointer to the new object or NULL if memory could not be allocated (no space) */
CIG_OPTIONAL(void*) cig_state_allocate(size_t);

/*  Similar to allocation, but for simply reading the arena.
    @param from_start - Resets read position to 0 before reading
    @return Pointer to the new object or NULL if memory could not be allocated (no space) */
CIG_OPTIONAL(void*) cig_state_read(bool from_start, size_t);

/*  ┌──────────────────────────────┐
    │ TEMPORARY BUFFERS (ADVANCED) │
    └──────────────────────────────┘ */

/*  Similar to `cig_begin_layout` where you start rendering into a new buffer,
    in the current element's coordinate system. All subsequent elements `absolute_rect`-s
    are relative to this buffer/screen/texture.
    
    This is mostly when you want to cache
    some more complex widget, like a large text view or similar. You can internally
    check whether you need to redraw or just re-render the old buffer/screen/texture */  
void cig_push_buffer(cig_buffer_ref);

/*  Pops the previously pushed buffer. Does not reset anything else about the state
    of the UI, unlike `cig_end_layout` */
void cig_pop_buffer();

/*  ┌───────────────────────────┐
    │ INPUT INTERACTION & FOCUS │
    └───────────────────────────┘ */

/*  Pass input coordinates and state[s] */
void cig_set_input_state(cig_v, cig_input_action_type_t);

/*  @return Current input state as updated by last `cig_set_input_state` call */
CIG_OPTIONAL(cig_input_state_t*) cig_input_state();

/*  Enables input tracking for the current layout element.
    Call this after a successful `cig_push_frame` call */
void cig_enable_interaction();

/*  Checks if the current layout element is the topmost element at current input position */
bool cig_hovered();

/*  Checks if the current element is hovered and the mouse button is pressed.
    See `cig_press_flags_t` declaration for more info */
cig_input_action_type_t cig_pressed(cig_input_action_type_t, cig_press_flags_t);

/*  Checks if the current element is hovered and mouse button was clicked or released
    depending on the options. See `cig_click_flags_t` declaration for more info */
cig_input_action_type_t cig_clicked(cig_input_action_type_t, cig_click_flags_t);

/*  Makes this frame focusable. Focus is gained by pressing mouse button in this frame
    regardless of interaction being enabled or not. Returns whether the same element
    was focused last */
bool cig_enable_focus();

/*  Returns whether this frame is the current focus target. Requires `cig_enable_focus` to
    have been called */
bool cig_focused();

/* */
cig_id_t cig_focused_id();

/*  Explicitly set focus to given frame ID */
void cig_set_focused_id(cig_id_t);

/*  ┌───────────┐
    │ SCROLLING │
    └───────────┘ */

/*  Attempts to enable scrolling for the current layout element and allocate an
    internal scroll state that remains constant between ticks. You can also provide
    a pointer to the struct stored somewhere in your application layer. Pass NULL for
    the default behavior described above.
    
    The state pool is limited to `CIG_STATES_MAX` and if there are too many
    scrolling elements already, it may fail.
    
    @return TRUE if state could be allocated, FALSE otherwise */
bool cig_enable_scroll(cig_scroll_state_t *);

/*  @return Current scroll state objet, or NULL if scrolling is not enabled */
CIG_OPTIONAL(cig_scroll_state_t*) cig_scroll_state();

/*  Set scroll offset values */
void cig_set_offset(cig_v);

/*  Change scroll offset values */
void cig_change_offset(cig_v);

/*  @return Scroller offset */
cig_v cig_offset();

/*  ┌────────────────┐
    │ LAYOUT HELPERS │
    └────────────────┘ */

/*  By default, when adding rects that are completely outside the bounds
    of the parent, `cig_push_frame` calls return NULL. You can disable that
    behavior with this */
void cig_disable_culling();

/*  Enables clipping for the current layout element */
void cig_enable_clipping();

/*  Normally element ID is auto-calculated and may vary from tick to tick.
    This sets an explicit Id for the next `cig_push_frame` call.
    See `cig_hash` for generating an ID from a string */
void cig_set_next_id(cig_id_t);

/*  Depth of the current layout stack */
unsigned int cig_depth();

/*  Generates an ID from a string */
cig_id_t cig_hash(const char *str);

/*  Pushes and pops an empty frame to trigger a layout function to allocate space.
    Useful when you have a stack or grid and want to trigger a new line or column */
void cig_empty();

/* */
void cig_spacer(int size);

/*  Default layout function for stack and grid type */
bool cig_default_layout_builder(cig_r, cig_r, cig_layout_params_t*, cig_r*);

cig_frame_t* cig_push_hstack(cig_r, cig_i, cig_layout_params_t);

cig_frame_t* cig_push_vstack(cig_r, cig_i, cig_layout_params_t);

cig_frame_t* cig_push_grid(cig_r, cig_i, cig_layout_params_t);

/*  ┌───────────────────┐
    │ BACKEND CALLBACKS │
    └───────────────────┘ */

void cig_set_clip_rect_callback(cig_set_clip_rect_callback_t);

#ifdef DEBUG

/*  ┌────────────┐
    │ DEBUG MODE │
    └────────────┘ */

typedef void (*cig_layout_breakpoint_callback_t)(cig_r, cig_r);

void cig_set_layout_breakpoint_callback(cig_layout_breakpoint_callback_t);

/*  Starts stepping through the hierarchy starting on next layout pass*/
void cig_enable_debug_stepper();

/*  Can be called during step-through to cancel and go back to real-time rendering */
void cig_disable_debug_stepper();

/*  If step mode is active, triggers a breakpoint you can use to
    visualize the layout as it currently stands. Two rectangles
    indicate what is being laid out into what */
void cig_trigger_layout_breakpoint(cig_r container, cig_r rect);

#endif

#endif
