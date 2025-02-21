#ifndef VEC2_H
#define VEC2_H

#include "macros.h"
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
// #include <libm/math.h>

#define VEC2_T int
#define VEC2(X, Y) (vec2){X, Y}
#define VEC2_CONST(X,Y) {X,Y}

typedef struct vec2 {
	VEC2_T x;
	VEC2_T y;
} vec2;

FASTFUNC vec2 vec2_make(VEC2_T x, VEC2_T y) {
	return (vec2){x, y};
}

FASTFUNC vec2 vec2_zero() {
	return (vec2){0, 0};
}

FASTFUNC vec2 vec2_invalid() {
	return (vec2){999, 999};
}

FASTFUNC vec2 vec2_add(vec2 a, vec2 b) {
	return (vec2){a.x + b.x, a.y + b.y};
}

FASTFUNC vec2 vec2_sub(vec2 a, vec2 b) {
	return (vec2){a.x - b.x, a.y - b.y};
}

FASTFUNC vec2 vec2_mul(vec2 a, VEC2_T f) {
	return (vec2){a.x * f, a.y * f};
}

FASTFUNC bool vec2_valid(vec2 a) {
	return (a.x != 999 && a.y != 999);
}

FASTFUNC bool vec2_cmp(vec2 a, vec2 b) {
	return (a.x == b.x && a.y == b.y);
}

FASTFUNC vec2 vec2_normalize_isometric(const vec2 v) {
	return vec2_make((v.x / 2) * 2, v.y);
}

FASTFUNC int vec2_sign(const vec2 P, const vec2 A, const vec2 B) {
  return (P.x - B.x) * (A.y - B.y) - (A.x - B.x) * (P.y - B.y);
}

FASTFUNC int vec2_distsq(const vec2 A, const vec2 B) {
	return (pow(fabs((double)(B.x-A.x)), 2) + pow(fabs((double)(B.y-A.y)), 2));
}

FASTFUNC vec2 vec2_tile_offset_direction(const vec2 v, const int dir) {
	switch (dir) {
		case 0: return vec2_add(v, vec2_make(0,-1));
		case 1: return vec2_add(v, vec2_make(-1,0));
		case 2: return vec2_add(v, vec2_make(0,1));
		default: return vec2_add(v, vec2_make(1,0));
	}
}

FASTFUNC vec2 vec2_offset_direction(const vec2 v, const int dir, const vec2 unit) {
	switch (dir) {
		case 0: return vec2_add(v, vec2_make(-unit.x, -unit.y));
		case 1: return vec2_add(v, vec2_make(-unit.x, unit.y));
		case 2: return vec2_add(v, vec2_make(unit.x, unit.y));
		default: return vec2_add(v, vec2_make(unit.x, -unit.y));
	}
}

FASTFUNC vec2 vec2_tile_offset_direction_multiply(
	const vec2 v,
	const int dir,
	const int m
) {
	switch (dir) {
		case 0: return vec2_add(v, vec2_make(0,-1*m));
		case 1: return vec2_add(v, vec2_make(-1*m,0));
		case 2: return vec2_add(v, vec2_make(0,1*m));
		default: return vec2_add(v, vec2_make(1*m,0));
	}
}

vec2 vec2_abs(vec2);
vec2 vec2_lerp (const vec2, const vec2, const float);
vec2 vec2_to_tile_coords(const vec2);

#endif
