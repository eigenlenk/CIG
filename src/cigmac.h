#ifndef CIG_MACROS_DEFINED
#define CIG_MACROS_DEFINED

#define CIG_MAX(x, y) (((x) > (y)) ? (x) : (y))
#define CIG_MIN(x, y) (((x) < (y)) ? (x) : (y))
#define CASE(I, NAME) NAME = I
#define CASEFLAG(B, NAME) NAME = (1 << B)

#endif