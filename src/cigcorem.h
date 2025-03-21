#ifndef IM_MACROS_H
#define IM_MACROS_H

/* ╔══════════════════════════════════════════╗
   ║ BASIC CORE MACROS                        ║
	 ║                                          ║
	 ║ Collection of (in)convenience macros for ║
	 ║ dealing with layout and sizing           ║
   ╚══════════════════════════════════════════╝ */
	 

#define IM_FILL_CONSTANT 0

#define IM_FILL frame_make(0, 0, IM_FILL_CONSTANT, IM_FILL_CONSTANT)

#define IM_FILL_W(W) frame_make(0, 0, W, IM_FILL_CONSTANT)

#define IM_FILL_H(H) frame_make(0, 0, IM_FILL_CONSTANT, H)

#define IM_X im_current_element()->frame.x

#define IM_Y im_current_element()->frame.y

#define IM_W im_current_element()->frame.w

#define IM_H im_current_element()->frame.h

#define IM_CENTER vec2_make((IM_W * 0.5), (IM_H * 0.5))

#define IM_CENTERED(W, H) frame_make((IM_W * 0.5) - (W * 0.5), (IM_H * 0.5) - (H * 0.5), W, H)

#define IM_R (IM_X + IM_W)

#define IM_B (IM_Y + IM_H)

#define CIM_SCROLL_LIMIT_X im_current_element()->_scroll_state->content_size.x-im_current_element()->frame.w+im_current_element()->insets.left+im_current_element()->insets.right

#define CIM_SCROLL_LIMIT_Y im_current_element()->_scroll_state->content_size.y-im_current_element()->frame.h+im_current_element()->insets.top+im_current_element()->insets.bottom

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
	
#define IM_FILLED(BODY) \
	if (im_push_frame(IM_FILL)) { \
		BODY; \
		im_pop_frame(); \
	}
	
#define IM_BODY(CONTENT) CONTENT


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

#define IM_LAYOUT_PARAMS(PARAMS...) (im_layout_params_t) { 0, PARAMS }

#endif
