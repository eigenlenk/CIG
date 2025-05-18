#ifndef CIG_MACROS_INCLUDED
#define CIG_MACROS_INCLUDED

#define CIG_MAX(x, y) (((x)>(y))?(x):(y))
#define CIG_MIN(x, y) (((x)<(y))?(x):(y))
#define CIG_CLAMP(N, L, U) CIG_MAX(CIG_MIN(U, N), L)
#define CIG_INLINED static inline __attribute__((always_inline))
#define CIG_PACKED __attribute__((__packed__))
#define CIG_BIT(B) (1<<B)
#define CIG_UNUSED(x) (void)(x)
#define CIG_OPTIONAL(X) X
#define CIG_DISCARDABLE(X) X

// https://stackoverflow.com/questions/2124339/c-preprocessor-va-args-number-of-arguments
#define CIG_NARG(...) CIG__NARG_(_,##__VA_ARGS__,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0)
#define CIG__NARG_(_,...) CIG__ARG_N(__VA_ARGS__)
#define CIG__ARG_N(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,N,...) N

#endif