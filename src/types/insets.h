#ifndef CIG_TYPE_INSETS_T_INCLUDED
#define CIG_TYPE_INSETS_T_INCLUDED

#include "cigmac.h"

/*  Macro for declaring a insets<T> type */
#define DECLARE_INSETS_T(T, DECLNAME)                                        \
                                                                             \
typedef struct {                                                             \
  T left, top, right, bottom;                                                \
} DECLNAME##_t;                                                              \
                                                                             \
CIG_INLINED DECLNAME##_t DECLNAME##_make(T left, T top, T right, T bottom) { \
  return (DECLNAME##_t) { left, top, right, bottom };                        \
}                                                                            \
                                                                             \
CIG_INLINED DECLNAME##_t DECLNAME##_zero() {                                 \
  return (DECLNAME##_t) { 0 };                                               \
}                                                                            \
                                                                             \
CIG_INLINED DECLNAME##_t DECLNAME##_uniform(T inset) {                       \
  return (DECLNAME##_t) { inset, inset, inset, inset };                      \
}                                                                            \
                                                                             \
CIG_INLINED DECLNAME##_t DECLNAME##_horizontal(T inset) {                    \
  return (DECLNAME##_t) { inset, 0, inset, 0 };                              \
}                                                                            \
                                                                             \
CIG_INLINED DECLNAME##_t DECLNAME##_vertical(T inset) {                      \
  return (DECLNAME##_t) { 0, inset, 0, inset };                              \
}

#endif
