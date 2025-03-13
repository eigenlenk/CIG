#ifndef FRAME_T_H
#define FRAME_T_H

#include "macros.h"
#include "types/vec2.h"
#include "types/insets.h"

#define T int

typedef struct frame_t {
	T x, y, w, h;
} frame_t;

FASTFUNC frame_t frame_make(T x, T y, T w, T h) {
  return (frame_t) { x, y, w, h };
}

FASTFUNC frame_t frame_zero() {
	return (frame_t) { 0 };
}

FASTFUNC bool frame_contains(const frame_t frame, const vec2 point) {
  return (
    point.x >= frame.x
    && point.y >= frame.y
    && point.x < frame.x + frame.w
    && point.y < frame.y + frame.h
  );
}

FASTFUNC frame_t frame_expand(const frame_t frame, const T n) {
  return frame_make(frame.x - n, frame.y - n, frame.w + 2 * n, frame.h + 2 * n);
}

FASTFUNC frame_t frame_inset(const frame_t frame, const insets_t insets) {
  return frame_make(
    frame.x + insets.left,
    frame.y + insets.top,
    frame.w - (insets.left + insets.right),
    frame.h - (insets.top + insets.bottom)
  );
}

FASTFUNC frame_t frame_resize(const frame_t frame, const T top, const T left, const T bottom, const T right) {
  return (frame_t){ frame.x - left, frame.y - top, frame.w + right, frame.h + bottom };
}

FASTFUNC frame_t frame_offset_vec2(const frame_t frame, const vec2 offset) {
  return (frame_t){ frame.x + offset.x, frame.y + offset.y, frame.w, frame.h };
}

FASTFUNC frame_t frame_offset(const frame_t frame, const T x, const T y) {
  return (frame_t){ frame.x + x, frame.y + y, frame.w, frame.h };
}

FASTFUNC bool frame_wholly_contains_relative_frame(const frame_t frame, const frame_t other) {
	return !(other.y < 0 || other.y + other.h > frame.h || other.x < 0 || other.x + other.w > frame.w || !other.w || !other.h);
}

FASTFUNC bool frame_intersects(const frame_t a, const frame_t b) {
	return !(a.x + a.w < b.x || a.y + a.h < b.y || a.x > b.x + b.w || a.y > b.y + b.h);
}

FASTFUNC bool frame_cmp(const frame_t lh, const frame_t rh) {
	return lh.x == rh.x && lh.y == rh.y && lh.w == rh.w && lh.h == rh.h;
}

FASTFUNC vec2 frame_center(const frame_t frame) {
	return VEC2(frame.x + frame.w * 0.5, frame.y + frame.h * 0.5);
}

/*FASTFUNC bool frame_is_empty(const frame_t frame) {
	return !(frame.w > 0 || frame.h > 0);
}*/

FASTFUNC bool frames_touching(const frame_t a, const frame_t b) {
	// if (a.y + a.h == b.y
	
	if (a.x <= b.x + b.w && a.x + a.w >= b.x && a.y <= b.y + b.h && a.y + a.h >= b.y) {
		return true;
	}
	
	return false;
}

FASTFUNC frame_t frame_containing(const frame_t a, const frame_t b) {
	T x = MIN(a.x, b.x), y = MIN(a.y, b.y);
	T w = MAX(a.x + a.w, b.x + b.w), h = MAX(a.y + a.h, b.y + b.h);
	return (frame_t) { x, y, w-x, h-y };
}

FASTFUNC frame_t frame_union(const frame_t a, const frame_t b) {
  if (frame_intersects(a, b)) {
    T x0 = MAX(a.x, b.x), y0 = MAX(a.y, b.y);
    T x1 = MIN(a.x+a.w, b.x+b.w), y1 = MIN(a.y+a.h, b.y+b.h);
    return (frame_t) { x0, y0, x1-x0, y1-y0 };
  } else {
    return frame_zero();
  }
}

#undef T

#endif
