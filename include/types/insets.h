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

FASTFUNC insets_t insets_zero() {
	return (insets_t) { 0 };
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

#undef T

#endif
