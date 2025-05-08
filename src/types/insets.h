#ifndef CIG_TYPE_INSETS_T_INCLUDED
#define CIG_TYPE_INSETS_T_INCLUDED

#include "cigmac.h"

/*  Macro for declaring a insets<T> type */
#define DECLARE_INSETS_T(T, DECLNAME)                                        \
                                                                             \
typedef struct {                                                             \
  T left, top, right, bottom;                                                \
} DECLNAME;                                                              \
                                                                             \
CIG_INLINED DECLNAME DECLNAME##_make(T left, T top, T right, T bottom) { \
  return (DECLNAME) { left, top, right, bottom };                        \
}                                                                            \
                                                                             \
CIG_INLINED DECLNAME DECLNAME##_zero() {                                 \
  return (DECLNAME) { 0 };                                               \
}                                                                            \
                                                                             \
CIG_INLINED DECLNAME DECLNAME##_uniform(T inset) {                       \
  return (DECLNAME) { inset, inset, inset, inset };                      \
}                                                                            \
                                                                             \
CIG_INLINED DECLNAME DECLNAME##_horizontal(T inset) {                    \
  return (DECLNAME) { inset, 0, inset, 0 };                              \
}                                                                            \
                                                                             \
CIG_INLINED DECLNAME DECLNAME##_vertical(T inset) {                      \
  return (DECLNAME) { 0, inset, 0, inset };                              \
}

#endif
