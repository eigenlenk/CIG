#ifndef CIG_CORE_MACROS_INCLUDED
#define CIG_CORE_MACROS_INCLUDED

#include "cigcore.h"

/* ╔══════════════════════════════════════════╗
   ║ CIG CORE MACROS                          ║
	 ║                                          ║
	 ║ Collection of (in)convenience macros for ║
	 ║ dealing with layout and sizing           ║
   ╚══════════════════════════════════════════╝ */
	 

#define CIG_FILL_CONSTANT 0

#define CIG_FILL cig_rect_make(0, 0, CIG_FILL_CONSTANT, CIG_FILL_CONSTANT)

#define CIG_FILL_W(W) cig_rect_make(0, 0, W, CIG_FILL_CONSTANT)

#define CIG_FILL_H(H) cig_rect_make(0, 0, CIG_FILL_CONSTANT, H)

#define CIG_X cig_rect().x

#define CIG_Y cig_rect().y

#define CIG_W cig_rect().w

#define CIG_H cig_rect().h

#define CIG_X_INSET (cig_rect().x + cig_frame()->insets.left)

#define CIG_Y_INSET (cig_rect().y + cig_frame()->insets.top)

#define CIG_W_INSET (CIG_W - cig_frame()->insets.left - cig_frame()->insets.right)

#define CIG_H_INSET (CIG_H - cig_frame()->insets.top - cig_frame()->insets.bottom)

#define CIG_GX cig_absolute_rect().x

#define CIG_GY cig_absolute_rect().y

#define CIG_CENTER cig_vec2_make((CIG_W * 0.5), (CIG_H * 0.5))

#define CIG_CENTERED(W, H) cig_rect_make((CIG_W * 0.5) - (W * 0.5), (CIG_H * 0.5) - (H * 0.5), W, H)

#define RECT_CENTERED_VERTICALLY(R) cig_rect_make(R.x, (CIG_H * 0.5) - (R.h * 0.5), R.w, R.h)

#define CIG_SIZED(W, H) cig_rect_make(0, 0, W, H)

#define CIG_R (CIG_X + CIG_W)

#define CIG_B (CIG_Y + CIG_H)

#define CIG_SCROLL_LIMIT_X cig_content_size().x-CIG_W+cig_frame()->insets.left+cig_frame()->insets.right

#define CIG_SCROLL_LIMIT_Y cig_content_size().y-CIG_H+cig_frame()->insets.top+cig_frame()->insets.bottom

#define CIG_ARRANGE(RECT, BODY) \
if (cig_push_frame(RECT)) { \
  BODY; \
  cig_pop_frame(); \
}

#define CIG_ARRANGE_WITH(RECT, INSETS, PARAMS, BODY) \
if (cig_push_frame_insets_params(RECT, INSETS, PARAMS)) { \
  BODY; \
  cig_pop_frame(); \
}
	
#define CIG_FILLED(BODY) \
if (cig_push_frame(CIG_FILL)) { \
  BODY; \
  cig_pop_frame(); \
}

#define CIG_BODY(CONTENT) CONTENT

/* `cig_layout_params_t` fields */
#define CIG_AXIS(A) .axis = A
#define CIG_DIRECTION(A) .direction = A
#define CIG_ALIGNMENT_HORIZONTAL(A) .alignment.horizontal = A
#define CIG_ALIGNMENT_VERTICAL(A) .alignment.vertical = A
#define CIG_SPACING(N) .spacing = N
#define CIG_WIDTH(W) .width = W
#define CIG_HEIGHT(H) .height = H
#define CIG_ROWS(N) .rows = N
#define CIG_COLUMNS(N) .columns = N
#define CIG_FLAGS(F) .flags = F
#define CIG_LIMIT_TOTAL(L) .limit.total = L
#define CIG_LIMIT_HORIZONTAL(L) .limit.horizontal = L
#define CIG_LIMIT_VERTICAL(L) .limit.vertical = L
#define CIG_MAX_WIDTH(N) .size_max.width = N
#define CIG_MAX_HEIGHT(N) .size_max.height = N
#define CIG_MIN_WIDTH(N) .size_min.width = N
#define CIG_MIN_HEIGHT(N) .size_min.height = N

/* `cig_frame_args_t` fields */
#define CIG_RECT(R) .rect = R
#define CIG_INSETS(I) .insets = I
#define CIG_PARAMS(P...) .params = (cig_layout_params_t) P
#define CIG_BUILDER(F) .builder = F

#define CIG_STACK_BUILDER &cig_default_layout_builder
#define CIG_GRID_BUILDER &cig_default_layout_builder

#define CIG_ALLOCATE(T) (T*)cig_state_allocate(sizeof(T));

#define _ {}

CIG_INLINED cig_frame_args_t cig__into_hstack(cig_frame_args_t args) {
  args = (cig_frame_args_t) {
    .rect = args.rect,
    .insets = args.insets,
    .params = args.params,
    .builder = &cig_default_layout_builder
  };
  args.params.axis = CIG_LAYOUT_AXIS_HORIZONTAL;
  return args;
}

CIG_INLINED cig_frame_args_t cig__into_vstack(cig_frame_args_t args) {
  args = (cig_frame_args_t) {
    .rect = args.rect,
    .insets = args.insets,
    .params = args.params,
    .builder = &cig_default_layout_builder
  };
  args.params.axis = CIG_LAYOUT_AXIS_VERTICAL;
  return args;
}

CIG_INLINED cig_frame_args_t cig__into_grid(cig_frame_args_t args) {
  args = (cig_frame_args_t) {
    .rect = args.rect,
    .insets = args.insets,
    .params = args.params,
    .builder = &cig_default_layout_builder
  };
  args.params.axis = CIG_LAYOUT_AXIS_HORIZONTAL | CIG_LAYOUT_AXIS_VERTICAL;
  return args;
}

#define CIG_HSTACK(ARGS...) for (int __push=1; (__push--)&&cig_push_frame_args(cig__into_hstack((cig_frame_args_t)ARGS)); cig_pop_frame())
#define CIG_VSTACK(ARGS...) for (int __push=1; (__push--)&&cig_push_frame_args(cig__into_vstack((cig_frame_args_t)ARGS)); cig_pop_frame())
#define CIG_GRID(ARGS...) for (int __push=1; (__push--)&&cig_push_frame_args(cig__into_grid((cig_frame_args_t)ARGS)); cig_pop_frame())

/* This calls the push_frame function once, and if it returns TRUE,
   eventually pops the frame as well */
#define CIG(ARGS...) for (int __push=1; (__push--)&&cig_push_frame_args((cig_frame_args_t)ARGS); cig_pop_frame())

#endif
