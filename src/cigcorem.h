#ifndef CIG_CORE_MACROS_INCLUDED
#define CIG_CORE_MACROS_INCLUDED

/* ╔══════════════════════════════════════════╗
   ║ CIG CORE MACROS                          ║
	 ║                                          ║
	 ║ Collection of (in)convenience macros for ║
	 ║ dealing with layout and sizing           ║
   ╚══════════════════════════════════════════╝ */
	 

#define CIG_FILL_CONSTANT 0

#define CIG_FILL cig_frame_make(0, 0, CIG_FILL_CONSTANT, CIG_FILL_CONSTANT)

#define CIG_FILL_W(W) cig_frame_make(0, 0, W, CIG_FILL_CONSTANT)

#define CIG_FILL_H(H) cig_frame_make(0, 0, CIG_FILL_CONSTANT, H)

#define CIG_X im_frame().x

#define CIG_Y im_frame().y

#define CIG_W im_frame().w

#define CIG_H im_frame().h

#define CIG_CENTER cig_vec2_make((IM_W * 0.5), (IM_H * 0.5))

#define CIG_CENTERED(W, H) cig_frame_make((IM_W * 0.5) - (W * 0.5), (IM_H * 0.5) - (H * 0.5), W, H)

#define CIG_R (IM_X + IM_W)

#define CIG_B (IM_Y + IM_H)

#define CIG_SCROLL_LIMIT_X im_content_size().x-im_frame().w+im_element()->insets.left+im_element()->insets.right

#define CIG_SCROLL_LIMIT_Y im_content_size().y-im_frame().h+im_element()->insets.top+im_element()->insets.bottom

#define IM_ARRANGE(FRAME, BODY) \
if (cig_push_frame(FRAME)) { \
  BODY; \
  cig_pop_frame(); \
}

#define IM_ARRANGE_INSETS(FRAME, INSETS, BODY) \
if (cig_push_frame_insets(FRAME, INSETS)) { \
  BODY; \
  cig_pop_frame(); \
}
	
#define CIG_FILLED(BODY) \
if (cig_push_frame(CIG_FILL)) { \
  BODY; \
  cig_pop_frame(); \
}

#define IM_BODY(CONTENT) CONTENT

#define IM_GRID(ID, COLUMNS, ROWS, SPACING, MOUSE, BODY) \
im_begin_grid(ID, COLUMNS, ROWS, SPACING, MOUSE); \
BODY \
im_end_grid();
	
#define IM_STACK(FRAME, BODY, AXIS, OPTIONS...) \
im_push_layout_frame_insets(FRAME, cig_insets_zero(), &im_layout_stack, (im_layout_params_t) { \
  0, \
  AXIS, \
  OPTIONS \
}); \
BODY; \
im_pop_frame();

#define IM_HSTACK(FRAME, BODY, OPTIONS...) \
im_push_layout_frame_insets(FRAME, cig_insets_zero(), &im_layout_stack, (im_layout_params_t) { \
  0, \
  .axis = HORIZONTAL, \
  OPTIONS \
}); \
BODY; \
im_pop_frame();

#define IM_VSTACK(FRAME, BODY, OPTIONS...) \
im_push_layout_frame_insets(FRAME, cig_insets_zero(), &im_layout_stack, (im_layout_params_t) { \
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
