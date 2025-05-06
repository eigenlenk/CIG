#ifndef CIG_TYPE_VEC2_T_INCLUDED
#define CIG_TYPE_VEC2_T_INCLUDED

#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

/*  Macro for declaring a vec2<T> type */
#define DECLARE_VEC2_T(T, DECLNAME, INVALID_CONST)                                \
                                                                                  \
typedef struct {                                                                  \
  T x;                                                                            \
  T y;                                                                            \
} DECLNAME;                                                                   \
                                                                                  \
CIG_INLINED DECLNAME DECLNAME##_make(T x, T y) {                              \
	return (DECLNAME){ x, y };                                                  \
}                                                                                 \
                                                                                  \
CIG_INLINED DECLNAME DECLNAME##_zero() {                                      \
	return (DECLNAME){ 0, 0 };                                                  \
}                                                                                 \
                                                                                  \
CIG_INLINED DECLNAME DECLNAME##_invalid() {                                   \
	return (DECLNAME) { INVALID_CONST, INVALID_CONST };                         \
}                                                                                 \
                                                                                  \
CIG_INLINED DECLNAME DECLNAME##_add(DECLNAME a, DECLNAME b) {         \
	return (DECLNAME) { a.x+b.x, a.y+b.y };                                     \
}                                                                                 \
                                                                                  \
CIG_INLINED DECLNAME DECLNAME##_sub(DECLNAME a, DECLNAME b) {         \
	return (DECLNAME) { a.x-b.x, a.y-b.y };                                     \
}                                                                                 \
                                                                                  \
CIG_INLINED DECLNAME DECLNAME##_mul(DECLNAME a, T f) {                    \
	return (DECLNAME) { f*a.x, f*a.y };                                         \
}                                                                                 \
                                                                                  \
CIG_INLINED DECLNAME DECLNAME##_div(DECLNAME a, T f) {                    \
	return (DECLNAME) { a.x/f, a.y/f };                                         \
}                                                                                 \
                                                                                  \
CIG_INLINED bool DECLNAME##_valid(DECLNAME a) {                               \
	return (a.x != INVALID_CONST && a.y != INVALID_CONST);                          \
}                                                                                 \
                                                                                  \
CIG_INLINED bool DECLNAME##_equals(DECLNAME a, DECLNAME b) {                 \
	return (a.x == b.x && a.y == b.y);                                              \
}                                                                                 \
                                                                                  \
CIG_INLINED int DECLNAME##_sign(DECLNAME p, DECLNAME a, DECLNAME b) { \
  return (p.x-b.x)*(a.y-b.y)-(a.x-b.x)*(p.y-b.y);                                 \
}                                                                                 \
                                                                                  \
CIG_INLINED double DECLNAME##_dist(DECLNAME a, DECLNAME b) {              \
	return sqrt(pow(fabs((double)(b.x-a.x)), 2) + pow(fabs((double)(b.y-a.y)), 2)); \
}                                                                                 \
                                                                                  \
CIG_INLINED int DECLNAME##_distsq(DECLNAME a, DECLNAME b) {               \
	return (pow(fabs((double)(a.x-b.x)), 2) + pow(fabs((double)(b.y-a.y)), 2));     \
}                                                                                 \
                                                                                  \
CIG_INLINED DECLNAME DECLNAME##_abs(DECLNAME v) {                         \
	return (DECLNAME) { abs(v.x), abs(v.y) };                                   \
}

#endif
