#ifndef CIG_TYPE_STACK_T_INCLUDED
#define CIG_TYPE_STACK_T_INCLUDED

#include "cigmac.h"
#include <stddef.h>
#include <assert.h>

/* Macro for declaring a stack<T> type */
#define DECLARE_ARRAY_STACK_T(T)                                    \
                                                                    \
typedef struct stack_##T {                                          \
  T elements[STACK_CAPACITY_##T];                                   \
  size_t size;                                                      \
  size_t capacity;                                                  \
  void (*clear)(struct stack_##T*);                                 \
  void (*push)(struct stack_##T*, T);                               \
  T (*pop)(struct stack_##T*);                                      \
  T (*peek)(struct stack_##T*, size_t);                             \
  T* (*_pop)(struct stack_##T*);                                    \
  T* (*_peek)(struct stack_##T*, size_t);                           \
} stack_##T##_t;                                                    \
                                                                    \
CIG_INLINED void stack_##T##_clear(stack_##T##_t *s) {              \
  s->size = 0;                                                      \
}                                                                   \
                                                                    \
CIG_INLINED void stack_##T##_push(stack_##T##_t *s, T e) {          \
  assert(s->size < s->capacity);                                    \
  s->elements[s->size++] = e;                                       \
}                                                                   \
                                                                    \
CIG_INLINED T stack_##T##_pop(stack_##T##_t *s) {                   \
  assert(s->size > 0);                                              \
  return s->elements[--s->size];                                    \
}                                                                   \
                                                                    \
CIG_INLINED T stack_##T##_peek(stack_##T##_t *s, size_t offset) {   \
  assert(s->size > 0);                                              \
  return s->elements[s->size-1-offset];                             \
}                                                                   \
                                                                    \
CIG_INLINED T* stack_##T##__pop(stack_##T##_t *s) {                 \
  assert(s->size > 0);                                              \
  return &s->elements[--s->size];                                   \
}                                                                   \
                                                                    \
CIG_INLINED T* stack_##T##__peek(stack_##T##_t *s, size_t offset) { \
  assert(s->size > 0);                                              \
  return &s->elements[s->size-1-offset];                            \
}

#define INIT_STACK(T) (stack_##T##_t) { \
  0,                                    \
  .capacity = STACK_CAPACITY_##T,       \
  .clear = &stack_##T##_clear,          \
  .push = &stack_##T##_push,            \
  .pop = &stack_##T##_pop,              \
  .peek = &stack_##T##_peek,            \
  ._pop = &stack_##T##__pop,            \
  ._peek = &stack_##T##__peek           \
}

#endif
