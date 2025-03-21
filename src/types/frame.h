#ifndef FRAME_T_H
#define FRAME_T_H

#include "cigmac.h"
#include "types/vec2.h"
#include "types/insets.h"

#define T int

typedef struct frame_t {
	T x, y, w, h;
} frame_t;

static inline  __attribute__((always_inline)) frame_t frame_make(T x, T y, T w, T h) {
  return (frame_t) { x, y, w, h };
}

static inline  __attribute__((always_inline)) frame_t frame_zero() {
	return (frame_t) { 0 };
}

static inline  __attribute__((always_inline)) bool frame_contains(const frame_t frame, const vec2 point) {
  return (
    point.x >= frame.x
    && point.y >= frame.y
    && point.x < frame.x + frame.w
    && point.y < frame.y + frame.h
  );
}

static inline  __attribute__((always_inline)) frame_t frame_inset(const frame_t frame, const insets_t insets) {
  return frame_make(
    frame.x + insets.left,
    frame.y + insets.top,
    frame.w - (insets.left + insets.right),
    frame.h - (insets.top + insets.bottom)
  );
}

static inline  __attribute__((always_inline)) frame_t frame_offset_vec2(const frame_t frame, const vec2 offset) {
  return (frame_t){ frame.x + offset.x, frame.y + offset.y, frame.w, frame.h };
}

static inline  __attribute__((always_inline)) frame_t frame_offset(const frame_t frame, const T x, const T y) {
  return (frame_t){ frame.x + x, frame.y + y, frame.w, frame.h };
}

static inline  __attribute__((always_inline)) bool frame_wholly_contains_relative_frame(const frame_t frame, const frame_t other) {
	return !(other.y < 0 || other.y + other.h > frame.h || other.x < 0 || other.x + other.w > frame.w || !other.w || !other.h);
}

static inline  __attribute__((always_inline)) bool frame_intersects(const frame_t a, const frame_t b) {
	return !(a.x + a.w - 1 < b.x || a.y + a.h - 1 < b.y || a.x > b.x + b.w - 1 || a.y > b.y + b.h - 1);
}

static inline  __attribute__((always_inline)) bool frame_cmp(const frame_t lh, const frame_t rh) {
	return lh.x == rh.x && lh.y == rh.y && lh.w == rh.w && lh.h == rh.h;
}

static inline  __attribute__((always_inline)) vec2 frame_center(const frame_t frame) {
	return VEC2(frame.x + frame.w * 0.5, frame.y + frame.h * 0.5);
}

static inline  __attribute__((always_inline)) frame_t frame_containing(const frame_t a, const frame_t b) {
	T x = CIG_MIN(a.x, b.x), y = CIG_MIN(a.y, b.y);
	T w = CIG_MAX(a.x + a.w, b.x + b.w), h = CIG_MAX(a.y + a.h, b.y + b.h);
	return (frame_t) { x, y, w-x, h-y };
}

static inline  __attribute__((always_inline)) frame_t frame_union(const frame_t a, const frame_t b) {
  if (frame_intersects(a, b)) {
    T x0 = CIG_MAX(a.x, b.x), y0 = CIG_MAX(a.y, b.y);
    T x1 = CIG_MIN(a.x+a.w, b.x+b.w), y1 = CIG_MIN(a.y+a.h, b.y+b.h);
    return (frame_t) { x0, y0, x1-x0, y1-y0 };
  } else {
    return frame_zero();
  }
}

#undef T

#endif
