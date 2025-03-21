#ifndef TYPE_VEC2_T_INCLUDED
#define TYPE_VEC2_T_INCLUDED

#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#define DECLARE_VEC2_T(T, DECLNAME, INVALID_CONST)                                 \
                                                                                   \
typedef struct {                                                                   \
	T x;                                                                             \
	T y;                                                                             \
} DECLNAME;                                                                        \
                                                                                   \
CIG_INLINED DECLNAME DECLNAME##_make(T x, T y) {                                   \
	return (vec2){ x, y };                                                           \
}                                                                                  \
                                                                                   \
CIG_INLINED DECLNAME DECLNAME##_zero() {                                           \
	return (vec2){ 0, 0 };                                                           \
}                                                                                  \
                                                                                   \
CIG_INLINED DECLNAME DECLNAME##_invalid() {                                        \
	return (DECLNAME) { INVALID_CONST, INVALID_CONST };                     \
}                                                                                  \
                                                                                   \
CIG_INLINED DECLNAME DECLNAME##_add(DECLNAME a, DECLNAME b) {                      \
	return (DECLNAME) { a.x + b.x, a.y + b.y };                                       \
}                                                                                  \
                                                                                   \
CIG_INLINED DECLNAME DECLNAME##_sub(DECLNAME a, DECLNAME b) {                      \
	return (DECLNAME) { a.x - b.x, a.y - b.y };                                       \
}                                                                                  \
                                                                                   \
CIG_INLINED DECLNAME DECLNAME##_mul(DECLNAME a, T f) {                             \
	return (DECLNAME) { a.x * f, a.y * f };                                           \
}                                                                                  \
                                                                                   \
CIG_INLINED DECLNAME DECLNAME##_div(DECLNAME a, T f) {                             \
	return (DECLNAME) { a.x / f, a.y / f };                                           \
}                                                                                  \
                                                                                   \
CIG_INLINED bool DECLNAME##_valid(DECLNAME a) {                                    \
	return (a.x != INVALID_CONST && a.y != INVALID_CONST);                 \
}                                                                                  \
                                                                                   \
CIG_INLINED bool DECLNAME##_cmp(DECLNAME a, DECLNAME b) {                          \
	return (a.x == b.x && a.y == b.y);                                               \
}                                                                                  \
                                                                                   \
CIG_INLINED int DECLNAME##_sign(DECLNAME P, DECLNAME A, DECLNAME B) {              \
  return (P.x - B.x) * (A.y - B.y) - (A.x - B.x) * (P.y - B.y);                    \
}                                                                                  \
                                                                                   \
CIG_INLINED double DECLNAME##_dist(DECLNAME A, DECLNAME B) {                       \
	return sqrt(pow(fabs((double)(B.x-A.x)), 2) + pow(fabs((double)(B.y-A.y)), 2)); \
}                                                                                 \
                                                                                  \
CIG_INLINED int DECLNAME##_distsq(DECLNAME A, DECLNAME B) {                       \
	return (pow(fabs((double)(B.x-A.x)), 2) + pow(fabs((double)(B.y-A.y)), 2));     \
}                                                                                  \
                                                                                   \
CIG_INLINED DECLNAME DECLNAME##_abs(DECLNAME v) {                                  \
	return (DECLNAME) { abs(v.x), abs(v.y) };                                         \
}

#endif
