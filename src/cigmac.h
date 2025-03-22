#ifndef CIG_MACROS_DEFINED
#define CIG_MACROS_DEFINED

#define CIG_MAX(x, y) (((x) > (y)) ? (x) : (y))
#define CIG_MIN(x, y) (((x) < (y)) ? (x) : (y))
#define CIG_INLINED static inline __attribute__((always_inline))
#define CASE(I, NAME) NAME = I
#define CASEFLAG(B, NAME) NAME = (1 << B)
#define CIG_UNUSED(x) (void)(x)

#endif