#ifndef CIG_CORE_MACROS_INCLUDED
#define CIG_CORE_MACROS_INCLUDED

#include "cigcore.h"

/*  ╔══════════════════════════════════════════╗
    ║ CIG CORE MACROS                          ║
    ║                                          ║
    ║ Collection of (in)convenience macros for ║
    ║ dealing with layout and sizing           ║
    ╚══════════════════════════════════════════╝ */

#define CIG_X cig_rect().x
#define CIG_Y cig_rect().y
#define CIG_W cig_rect().w
#define CIG_H cig_rect().h
#define CIG_X_INSET (cig_rect().x + cig_frame()->insets.left)
#define CIG_Y_INSET (cig_rect().y + cig_frame()->insets.top)
#define CIG_W_INSET (CIG_W - cig_frame()->insets.left - cig_frame()->insets.right)
#define CIG_H_INSET (CIG_H - cig_frame()->insets.top - cig_frame()->insets.bottom)
#define CIG_SX cig_absolute_rect().x
#define CIG_SY cig_absolute_rect().y
#define CIG_CENTER cig_v_make((CIG_W * 0.5), (CIG_H * 0.5))
#define CIG_R (CIG_X + CIG_W)
#define CIG_B (CIG_Y + CIG_H)
#define CIG_SPACE cig_frame()->_layout_params.spacing

#define RECT_CENTERED(W, H) cig_r_make((CIG_W * 0.5) - (W * 0.5), (CIG_H * 0.5) - (H * 0.5), W, H)
#define RECT_CENTERED_VERTICALLY(R) cig_r_make(R.x, (CIG_H * 0.5) - (R.h * 0.5), R.w, R.h)

/*  `cig_layout_params_t` fields */
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

/*  `cig_frame_args_t` fields */
#define CIG_INSETS(I) .insets = I
#define CIG_PARAMS(P...) .params = (cig_layout_params_t) P
#define CIG_BUILDER(F) .builder = F

#define CIG_ALLOCATE(T) (T*)cig_state_allocate(sizeof(T))
#define CIG_READ(S,T) (T*)cig_state_read(S, sizeof(T))

#define _ RECT_AUTO
#define _W(W) RECT_AUTO_W(W)
#define _H(H) RECT_AUTO_H(H)

extern cig_frame_t *cig__macro_last_closed;

/*  This calls the push_frame function once, performs the function body and pops the frame */
#define CIG(RECT, ARGS...) cig__macro_last_closed=NULL; for (int __pushed=0; !(__pushed++)&&cig_push_frame_args((cig_frame_args_t) { .rect=RECT, ARGS }); cig__macro_last_closed=cig_pop_frame())

#define CIG_HSTACK(RECT, ARGS...) cig__macro_last_closed=NULL; for (struct { int pushed; cig_frame_args_t args; } __cig = { 0, (cig_frame_args_t) { .rect=RECT, ARGS } }; !(__cig.pushed++)&&cig_push_hstack(__cig.args.rect, __cig.args.insets, __cig.args.params); cig__macro_last_closed=cig_pop_frame())
#define CIG_VSTACK(RECT, ARGS...) cig__macro_last_closed=NULL; for (struct { int pushed; cig_frame_args_t args; } __cig = { 0, (cig_frame_args_t) { .rect=RECT, ARGS } }; !(__cig.pushed++)&&cig_push_vstack(__cig.args.rect, __cig.args.insets, __cig.args.params); cig__macro_last_closed=cig_pop_frame())
#define CIG_GRID(RECT, ARGS...) cig__macro_last_closed=NULL; for (struct { int pushed; cig_frame_args_t args; } __cig = { 0, (cig_frame_args_t) { .rect=RECT, ARGS } }; !(__cig.pushed++)&&cig_push_grid(__cig.args.rect, __cig.args.insets, __cig.args.params); cig__macro_last_closed=cig_pop_frame())

#define CIG_LAST() cig__macro_last_closed
#define CIG_CAPTURE(VAR, BODY) BODY; VAR=CIG_LAST();

/* Layout pinning */

#define LEFT_OF(FRAME) .relation = FRAME, .relation_attribute = LEFT
#define RIGHT_OF(FRAME) .relation = FRAME, .relation_attribute = RIGHT
#define TOP_OF(FRAME) .relation = FRAME, .relation_attribute = TOP
#define BOTTOM_OF(FRAME) .relation = FRAME, .relation_attribute = BOTTOM
#define LEFT_INSET_OF(FRAME) .relation = FRAME, .relation_attribute = LEFT_INSET
#define RIGHT_INSET_OF(FRAME) .relation = FRAME, .relation_attribute = RIGHT_INSET
#define TOP_INSET_OF(FRAME) .relation = FRAME, .relation_attribute = TOP_INSET
#define BOTTOM_INSET_OF(FRAME) .relation = FRAME, .relation_attribute = BOTTOM_INSET
#define WIDTH_OF(FRAME) .relation = FRAME, .relation_attribute = WIDTH
#define HEIGHT_OF(FRAME) .relation = FRAME, .relation_attribute = HEIGHT
#define CENTER_X_OF(FRAME) .relation = FRAME, .relation_attribute = CENTER_X
#define CENTER_Y_OF(FRAME) .relation = FRAME, .relation_attribute = CENTER_Y

#define OFFSET_BY(VALUE) .value = VALUE

#define PIN(VALUES...) (cig_pin_t) { VALUES }

/*  Macro for building a rectangle based on rules how to set concrete
    values for left, right, top and bottom edges, or what element to reference
    for calculating them. In the end, all edges need to be explicitly calculable,
    but LEFT + WIDTH, or CENTER_X + WIDTH, or even CENTER_X + RIGHT all suffice
    to work out what left and right postions are.

    BUILD_RECT(
      PIN(LEFT, 10),
      PIN(WIDTH_OF(some_frame)),
      PIN(CENTER_Y(some_frame), OFFSET_BY(20)),
      PIN(HEIGHT, WIDTH_OF(some_frame))
    ) */
#define BUILD_RECT(REFS...) cig_build_rect(CIG_NARG(REFS), (cig_pin_t[]) { REFS })

#endif
