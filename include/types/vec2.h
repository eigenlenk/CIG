#ifndef VEC2_H
#define VEC2_H

#include "macros.h"
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#define T int
#define VEC2(X, Y) (vec2){ X, Y }
#define VEC2_CONST(X,Y) { X, Y }
#define VEC2_INVALID_CONST -999999

typedef struct vec2 {
	T x;
	T y;
} vec2;

FASTFUNC vec2 vec2_make(T x, T y) {
	return (vec2){x, y};
}

FASTFUNC vec2 vec2_zero() {
	return (vec2){ 0, 0 };
}

FASTFUNC vec2 vec2_invalid() {
	return (vec2){ VEC2_INVALID_CONST, VEC2_INVALID_CONST };
}

FASTFUNC vec2 vec2_add(vec2 a, vec2 b) {
	return (vec2){a.x + b.x, a.y + b.y};
}

FASTFUNC vec2 vec2_sub(vec2 a, vec2 b) {
	return (vec2){a.x - b.x, a.y - b.y};
}

FASTFUNC vec2 vec2_mul(vec2 a, T f) {
	return (vec2){a.x * f, a.y * f};
}

FASTFUNC vec2 vec2_div(vec2 a, T f) {
	return (vec2){a.x / f, a.y / f};
}

FASTFUNC bool vec2_valid(vec2 a) {
	return (a.x != VEC2_INVALID_CONST && a.y != VEC2_INVALID_CONST);
}

FASTFUNC bool vec2_cmp(vec2 a, vec2 b) {
	return (a.x == b.x && a.y == b.y);
}

FASTFUNC int vec2_sign(const vec2 P, const vec2 A, const vec2 B) {
  return (P.x - B.x) * (A.y - B.y) - (A.x - B.x) * (P.y - B.y);
}

FASTFUNC int vec2_distsq(const vec2 A, const vec2 B) {
	return (pow(fabs((double)(B.x-A.x)), 2) + pow(fabs((double)(B.y-A.y)), 2));
}

FASTFUNC vec2 vec2_abs(vec2 v) {
	return (vec2){ abs(v.x), abs(v.y) };
}

#undef T

#endif
