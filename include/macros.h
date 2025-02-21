#ifndef MACROS_H
#define MACROS_H

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MAXMIN(L, U, V) MAX(L, MIN(U, V))
#define FASTFUNC static inline __attribute__((always_inline))
#define ALWAYS_INLINED inline __attribute__((always_inline))
#define UNUSED(x) (void)(x)
#define CASE(I, NAME) NAME = I
#define BITFLAG(B, NAME) NAME = (1 << B)

#endif
