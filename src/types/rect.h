#ifndef CIG_TYPE_RECT_T_INCLUDED
#define CIG_TYPE_RECT_T_INCLUDED

#include "cigmac.h"

/*  Macro for declaring a rect<T> type. VEC2 and INSETS define which
    type to use for some of the arguments and return values. Ideally
    they'd all use the same datatype */
#define DECLARE_RECT_T(T, DECLNAME, VEC2, INSETS)                                            \
                                                                                             \
typedef struct {                                                                             \
  T x, y, w, h;                                                                              \
} DECLNAME;                                                                              \
                                                                                             \
CIG_INLINED DECLNAME DECLNAME##_make(T x, T y, T w, T h) {                               \
  return (DECLNAME) { x, y, w, h };                                                      \
}                                                                                            \
                                                                                             \
CIG_INLINED DECLNAME DECLNAME##_zero() {                                                 \
  return (DECLNAME) { 0 };                                                               \
}                                                                                            \
                                                                                             \
CIG_INLINED bool DECLNAME##_contains(DECLNAME F, VEC2 P) {                           \
  return P.x >= F.x && P.y >= F.y && P.x < F.x + F.w && P.y < F.y + F.h;                     \
}                                                                                            \
                                                                                             \
CIG_INLINED  DECLNAME  DECLNAME##_inset(DECLNAME f, INSETS i) {                  \
  return (DECLNAME) {                                                                    \
    f.x + i.left,                                                                            \
    f.y + i.top,                                                                             \
    f.w - (i.left + i.right),                                                                \
    f.h - (i.top + i.bottom)                                                                 \
  };                                                                                         \
}                                                                                            \
                                                                                             \
CIG_INLINED DECLNAME DECLNAME##_offset_vec2(DECLNAME rect, VEC2 offset) {        \
  return (DECLNAME) { rect.x + offset.x, rect.y + offset.y, rect.w, rect.h };            \
}                                                                                            \
                                                                                             \
CIG_INLINED DECLNAME DECLNAME##_offset(DECLNAME rect, T x, T y) {                    \
  return (DECLNAME) { rect.x + x, rect.y + y, rect.w, rect.h };                          \
}                                                                                            \
                                                                                             \
CIG_INLINED bool DECLNAME##_intersects(DECLNAME a, DECLNAME b) {                     \
  return !(a.x+a.w-1 < b.x || a.y+a.h-1 < b.y || a.x > b.x+b.w-1 || a.y > b.y+b.h-1);        \
}                                                                                            \
                                                                                             \
CIG_INLINED bool DECLNAME##_equals(DECLNAME lh, DECLNAME rh) {                          \
  return lh.x == rh.x && lh.y == rh.y && lh.w == rh.w && lh.h == rh.h;                       \
}                                                                                            \
                                                                                             \
CIG_INLINED VEC2 DECLNAME##_center(DECLNAME f) {                                     \
  return VEC2##_make(f.x + f.w * 0.5, f.y + f.h * 0.5);                                      \
}                                                                                            \
                                                                                             \
CIG_INLINED VEC2 DECLNAME##_size(DECLNAME f) {                                     \
  return VEC2##_make(f.w, f.h);                                      \
}                                                                                            \
                                                                                             \
CIG_INLINED DECLNAME DECLNAME##_containing(DECLNAME a, DECLNAME b) {             \
  T x = CIG_MIN(a.x, b.x), y = CIG_MIN(a.y, b.y);                                            \
  T w = CIG_MAX(a.x + a.w, b.x + b.w), h = CIG_MAX(a.y + a.h, b.y + b.h);                    \
  return (DECLNAME) { x, y, w-x, h-y };                                                  \
}                                                                                            \
                                                                                             \
CIG_INLINED DECLNAME DECLNAME##_union(DECLNAME a, DECLNAME b) {                  \
  if (DECLNAME##_intersects(a, b)) {                                                         \
    T x0 = CIG_MAX(a.x, b.x), y0 = CIG_MAX(a.y, b.y);                                        \
    T x1 = CIG_MIN(a.x+a.w, b.x+b.w), y1 = CIG_MIN(a.y+a.h, b.y+b.h);                        \
    return (DECLNAME) { x0, y0, x1-x0, y1-y0 };                                          \
  } else {                                                                                   \
    return DECLNAME##_zero();                                                                \
  }                                                                                          \
}                           \
                                                                                             \
CIG_INLINED DECLNAME DECLNAME##_clip(DECLNAME rect, DECLNAME clip_rect) {                  \
  T x0 = CIG_MAX(rect.x, clip_rect.x); \
  T y0 = CIG_MAX(rect.y, clip_rect.y); \
  T x1 = CIG_MIN(rect.x+rect.w, clip_rect.w); \
  T y1 = CIG_MIN(rect.y+rect.h, clip_rect.h); \
  return (DECLNAME) { x0, y0, x1-x0, y1-y0 }; \
}

#endif
