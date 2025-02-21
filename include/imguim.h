#ifndef IM_MACROS_H
#define IM_MACROS_H

/*
 * Collection of (in)convenience macros for dealing with layout and sizing.
 */

#define IM_ARRANGE(FRAME, BODY) \
	if (im_push_frame(FRAME)) { \
		BODY; \
		im_pop_frame(); \
	}
	
	
#define IM_ARRANGE_INSETS(FRAME, INSETS, BODY) \
	if (im_push_frame_insets(FRAME, INSETS)) { \
		BODY; \
		im_pop_frame(); \
	}
	
#define IM_ARRANGE_FILL(BODY) \
	if (im_push_frame(IM_FILL)) { \
		BODY; \
		im_pop_frame(); \
	}
	
#define IM_BODY(CONTENT) CONTENT

/*
 * WINDOW MACROS
 */
#define IM_WINDOW(TITLE, POS, SIZE, DECORATIONS, VISIBLE, BUTTONS, BODY) \
	if (im_begin_window(TITLE, POS, SIZE, DECORATIONS, VISIBLE, BUTTONS)) { \
		BODY \
		im_end_window(); \
	}
#define IM_WINDOW_BUTTONS_EMPTY (im_window_button_t) { 0 }
#define IM_WINDOW_BUTTONS(BTNS...) BTNS, IM_WINDOW_BUTTONS_EMPTY
#define IM_WINDOW_BUTTON(TITLE, CALLBACK) (im_window_button_t) { .title = TITLE, .click = CALLBACK }
#define IM_WINDOW_CONTENT_SIZE(W, H) vec2_make(W, H)

#define IM_GRID(ID, COLUMNS, ROWS, SPACING, MOUSE, BODY) \
	im_begin_grid(ID, COLUMNS, ROWS, SPACING, MOUSE); \
	BODY \
	im_end_grid();
	
#define IM_STACK(FRAME, BODY, AXIS, OPTIONS...) \
	im_push_layout_frame_insets(FRAME, insets_zero(), &im_layout_stack, (im_layout_params_t) { \
		0, \
		AXIS, \
		OPTIONS \
	}); \
	BODY; \
	im_pop_frame();
	
#define IM_HSTACK(FRAME, BODY, OPTIONS...) \
	im_push_layout_frame_insets(FRAME, insets_zero(), &im_layout_stack, (im_layout_params_t) { \
		0, \
		.axis = HORIZONTAL, \
		OPTIONS \
	}); \
	BODY; \
	im_pop_frame();
	
#define IM_VSTACK(FRAME, BODY, OPTIONS...) \
	im_push_layout_frame_insets(FRAME, insets_zero(), &im_layout_stack, (im_layout_params_t) { \
		0, \
		.axis = VERTICAL, \
		OPTIONS \
	}); \
	BODY; \
	im_pop_frame();
	
#define IM_STACK_AXIS(A) .axis = A
#define IM_STACK_SPACING(N) .spacing = N
#define IM_STACK_WIDTH(W) .width = W
#define IM_STACK_HEIGHT(H) .height = H
#define IM_STACK_ROWS(N) .height = N
#define IM_STACK_COLUMNS(N) .width = N
#define IM_STACK_OPTIONS(F) .options = F

#define IM_MENU_SEPARATOR "-"

#define IM_LAYOUT_PARAMS(PARAMS...) (im_layout_params_t) { 0, PARAMS }

#define IM_MOUSE_LISTENER(ID, BODY, CLICKBODY) \
	if (im_mouse_listener(ID, IM_FILL, IM_MOUSE_PRESS_INSIDE, IM_MOUSE_BUTTON_LEFT, NULL, NULL, NULL)) { \
		CLICKBODY; \
	} \
	BODY;

#endif
