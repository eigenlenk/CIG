#ifndef INSETS_T_H
#define INSETS_T_H

#include "macros.h"

#define T short

typedef struct insets_t {
	T left, top, right, bottom;
} insets_t;

FASTFUNC insets_t insets_make(T left, T top, T right, T bottom) {
  return (insets_t) { left, top, right, bottom };
}

FASTFUNC insets_t insets_uniform(T inset) {
  return (insets_t) { inset, inset, inset, inset };
}

FASTFUNC insets_t insets_horizontal(T inset) {
  return (insets_t) { inset, 0, inset, 0 };
}

FASTFUNC insets_t insets_vertical(T inset) {
  return (insets_t) { 0, inset, 0, inset };
}

FASTFUNC insets_t insets_bottom(T inset) {
  return (insets_t) { 0, 0, 0, inset };
}

FASTFUNC insets_t insets_adding_top(insets_t insets, T add) {
  insets.top += add;
	return insets;
}

FASTFUNC insets_t insets_adding_bottom(insets_t insets, T add) {
  insets.bottom += add;
	return insets;
}

FASTFUNC insets_t insets_inverted(insets_t insets) {
  return (insets_t) { -insets.left, -insets.top, -insets.right, -insets.bottom };
}

FASTFUNC insets_t insets_zero() {
	return (insets_t) { 0 };
}

#undef T

#endif
