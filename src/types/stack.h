#ifndef STACK_H
#define STACK_H

#include "cigmac.h"
#include <stddef.h>
#include <assert.h>

#define DECLARE_ARRAY_STACK(T)                            \
typedef struct stack_##T {                                \
  T elements[STACK_CAPACITY_##T];                         \
  size_t size;                                            \
  size_t capacity;                                        \
  void (*clear)(struct stack_##T*);                       \
  void (*push)(struct stack_##T*, T);                     \
  T (*pop)(struct stack_##T*);                            \
  T (*peek)(struct stack_##T*, size_t);                   \
  T* (*_pop)(struct stack_##T*);                          \
  T* (*_peek)(struct stack_##T*, size_t);                 \
} stack_##T;                                              \
                                                          \
CIG_INLINED void stack_##T##_clear(struct stack_##T *s) { \
  s->size = 0;                                            \
}                                                         \
                                                          \
CIG_INLINED void stack_##T##_push(struct stack_##T *s, T e) { \
  s->elements[s->size++] = e;                             \
}                                                         \
                                                          \
CIG_INLINED T stack_##T##_pop(struct stack_##T *s) {                 \
  return s->elements[--s->size];                         \
}                                                         \
                                                          \
CIG_INLINED T stack_##T##_peek(struct stack_##T *s, size_t offset) { \
  assert(s->size > 0);                                    \
  return s->elements[s->size-offset-1];                  \
} \
                                                          \
CIG_INLINED T* stack_##T##__pop(struct stack_##T *s) {                 \
  return &s->elements[--s->size];                         \
}                                                         \
                                                          \
CIG_INLINED T* stack_##T##__peek(struct stack_##T *s, size_t offset) { \
  assert(s->size > 0);                                    \
  return &s->elements[s->size-offset-1];                  \
}

#define CONFIGURE_STACK(T)        \
(stack_##T) {                     \
  0,                              \
  .capacity = STACK_CAPACITY_##T, \
  .clear = &stack_##T##_clear,    \
  .push = &stack_##T##_push,      \
  .pop = &stack_##T##_pop,        \
  .peek = &stack_##T##_peek,      \
  ._pop = &stack_##T##__pop,      \
  ._peek = &stack_##T##__peek     \
}

#endif
