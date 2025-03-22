#ifndef CIG_TYPE_VEC2_T_INCLUDED
#define CIG_TYPE_VEC2_T_INCLUDED

#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#define DECLARE_VEC2_T(T, DECLNAME, INVALID_CONST)                                \
                                                                                  \
typedef struct {                                                                  \
  T x;                                                                            \
  T y;                                                                            \
} DECLNAME##_t;                                                                   \
                                                                                  \
CIG_INLINED DECLNAME##_t DECLNAME##_make(T x, T y) {                              \
	return (DECLNAME##_t){ x, y };                                                  \
}                                                                                 \
                                                                                  \
CIG_INLINED DECLNAME##_t DECLNAME##_zero() {                                      \
	return (DECLNAME##_t){ 0, 0 };                                                  \
}                                                                                 \
                                                                                  \
CIG_INLINED DECLNAME##_t DECLNAME##_invalid() {                                   \
	return (DECLNAME##_t) { INVALID_CONST, INVALID_CONST };                         \
}                                                                                 \
                                                                                  \
CIG_INLINED DECLNAME##_t DECLNAME##_add(DECLNAME##_t a, DECLNAME##_t b) {         \
	return (DECLNAME##_t) { a.x+b.x, a.y+b.y };                                     \
}                                                                                 \
                                                                                  \
CIG_INLINED DECLNAME##_t DECLNAME##_sub(DECLNAME##_t a, DECLNAME##_t b) {         \
	return (DECLNAME##_t) { a.x-b.x, a.y-b.y };                                     \
}                                                                                 \
                                                                                  \
CIG_INLINED DECLNAME##_t DECLNAME##_mul(DECLNAME##_t a, T f) {                    \
	return (DECLNAME##_t) { f*a.x, f*a.y };                                         \
}                                                                                 \
                                                                                  \
CIG_INLINED DECLNAME##_t DECLNAME##_div(DECLNAME##_t a, T f) {                    \
	return (DECLNAME##_t) { a.x/f, a.y/f };                                         \
}                                                                                 \
                                                                                  \
CIG_INLINED bool DECLNAME##_valid(DECLNAME##_t a) {                               \
	return (a.x != INVALID_CONST && a.y != INVALID_CONST);                          \
}                                                                                 \
                                                                                  \
CIG_INLINED bool DECLNAME##_cmp(DECLNAME##_t a, DECLNAME##_t b) {                 \
	return (a.x == b.x && a.y == b.y);                                              \
}                                                                                 \
                                                                                  \
CIG_INLINED int DECLNAME##_sign(DECLNAME##_t p, DECLNAME##_t a, DECLNAME##_t b) { \
  return (p.x-b.x)*(a.y-b.y)-(a.x-b.x)*(p.y-b.y);                                 \
}                                                                                 \
                                                                                  \
CIG_INLINED double DECLNAME##_dist(DECLNAME##_t a, DECLNAME##_t b) {              \
	return sqrt(pow(fabs((double)(b.x-a.x)), 2) + pow(fabs((double)(b.y-a.y)), 2)); \
}                                                                                 \
                                                                                  \
CIG_INLINED int DECLNAME##_distsq(DECLNAME##_t a, DECLNAME##_t b) {               \
	return (pow(fabs((double)(a.x-b.x)), 2) + pow(fabs((double)(b.y-a.y)), 2));     \
}                                                                                 \
                                                                                  \
CIG_INLINED DECLNAME##_t DECLNAME##_abs(DECLNAME##_t v) {                         \
	return (DECLNAME##_t) { abs(v.x), abs(v.y) };                                   \
}

#endif
