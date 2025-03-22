#ifndef CIG_TYPE_FRAME_T_INCLUDED
#define CIG_TYPE_FRAME_T_INCLUDED

#include "cigmac.h"

/* Macro for declaring a frame<T> type. VEC2 and INSETS define which
   type to use for some of the arguments and return values. Ideally
   they'd all use the same datatype */
#define DECLARE_FRAME_T(T, DECLNAME, VEC2, INSETS)                                           \
                                                                                             \
typedef struct {                                                                             \
  T x, y, w, h;                                                                              \
} DECLNAME##_t;                                                                              \
                                                                                             \
CIG_INLINED DECLNAME##_t DECLNAME##_make(T x, T y, T w, T h) {                               \
  return (DECLNAME##_t) { x, y, w, h };                                                      \
}                                                                                            \
                                                                                             \
CIG_INLINED DECLNAME##_t DECLNAME##_zero() {                                                 \
  return (DECLNAME##_t) { 0 };                                                               \
}                                                                                            \
                                                                                             \
CIG_INLINED bool DECLNAME##_contains(DECLNAME##_t F, VEC2##_t P) {                           \
  return P.x >= F.x && P.y >= F.y && P.x < F.x + F.w && P.y < F.y + F.h;                     \
}                                                                                            \
                                                                                             \
CIG_INLINED  DECLNAME##_t  DECLNAME##_inset(DECLNAME##_t f, INSETS##_t i) {                  \
  return (DECLNAME##_t) {                                                                    \
    f.x + i.left,                                                                            \
    f.y + i.top,                                                                             \
    f.w - (i.left + i.right),                                                                \
    f.h - (i.top + i.bottom)                                                                 \
  };                                                                                         \
}                                                                                            \
                                                                                             \
CIG_INLINED DECLNAME##_t DECLNAME##_offset_vec2(DECLNAME##_t frame, VEC2##_t offset) {       \
  return (DECLNAME##_t) { frame.x + offset.x, frame.y + offset.y, frame.w, frame.h };        \
}                                                                                            \
                                                                                             \
CIG_INLINED DECLNAME##_t DECLNAME##_offset(DECLNAME##_t frame, T x, T y) {                   \
  return (DECLNAME##_t) { frame.x + x, frame.y + y, frame.w, frame.h };                      \
}                                                                                            \
                                                                                             \
CIG_INLINED bool DECLNAME##_wholly_contains_relative_frame(DECLNAME##_t a, DECLNAME##_t b) { \
  return !(b.y < 0 || b.y+b.h > a.h || b.x < 0 || b.x+b.w > a.w || !b.w || !b.h);            \
}                                                                                            \
                                                                                             \
CIG_INLINED bool DECLNAME##_intersects(DECLNAME##_t a, DECLNAME##_t b) {                     \
  return !(a.x+a.w-1 < b.x || a.y+a.h-1 < b.y || a.x > b.x+b.w-1 || a.y > b.y+b.h-1);        \
}                                                                                            \
                                                                                             \
CIG_INLINED bool DECLNAME##_cmp(DECLNAME##_t lh, DECLNAME##_t rh) {                          \
  return lh.x == rh.x && lh.y == rh.y && lh.w == rh.w && lh.h == rh.h;                       \
}                                                                                            \
                                                                                             \
CIG_INLINED VEC2##_t DECLNAME##_center(DECLNAME##_t f) {                                     \
  return VEC2##_make(f.x + f.w * 0.5, f.y + f.h * 0.5);                                      \
}                                                                                            \
                                                                                             \
CIG_INLINED DECLNAME##_t DECLNAME##_containing(DECLNAME##_t a, DECLNAME##_t b) {             \
  T x = CIG_MIN(a.x, b.x), y = CIG_MIN(a.y, b.y);                                            \
  T w = CIG_MAX(a.x + a.w, b.x + b.w), h = CIG_MAX(a.y + a.h, b.y + b.h);                    \
  return (DECLNAME##_t) { x, y, w-x, h-y };                                                  \
}                                                                                            \
                                                                                             \
CIG_INLINED DECLNAME##_t DECLNAME##_union(DECLNAME##_t a, DECLNAME##_t b) {                  \
  if (DECLNAME##_intersects(a, b)) {                                                         \
    T x0 = CIG_MAX(a.x, b.x), y0 = CIG_MAX(a.y, b.y);                                        \
    T x1 = CIG_MIN(a.x+a.w, b.x+b.w), y1 = CIG_MIN(a.y+a.h, b.y+b.h);                        \
    return (DECLNAME##_t) { x0, y0, x1-x0, y1-y0 };                                          \
  } else {                                                                                   \
    return DECLNAME##_zero();                                                                \
  }                                                                                          \
}

#endif
