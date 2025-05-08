#ifndef CIG_TYPE_STACK_T_INCLUDED
#define CIG_TYPE_STACK_T_INCLUDED

#include "cigmac.h"
#include <stddef.h>
#include <assert.h>

/*  Macro for declaring a stack<T> type */
#define DECLARE_ARRAY_STACK_T(T)                                    \
                                                                    \
typedef struct T##_stack {                                          \
  T elements[STACK_CAPACITY_##T];                                   \
  size_t size;                                                      \
  size_t capacity;                                                  \
  void (*clear)(struct T##_stack*);                                 \
  void (*push)(struct T##_stack*, T);                               \
  T (*pop)(struct T##_stack*);                                      \
  T (*peek)(struct T##_stack*, size_t);                             \
  T* (*_pop)(struct T##_stack*);                                    \
  T* (*_peek)(struct T##_stack*, size_t);                           \
} T##_stack_t;                                                      \
                                                                    \
CIG_INLINED void stack_##T##_clear(T##_stack_t *s) {                \
  s->size = 0;                                                      \
}                                                                   \
                                                                    \
CIG_INLINED void stack_##T##_push(T##_stack_t *s, T e) {            \
  assert(s->size < s->capacity);                                    \
  s->elements[s->size++] = e;                                       \
}                                                                   \
                                                                    \
CIG_INLINED T stack_##T##_pop(T##_stack_t *s) {                     \
  assert(s->size > 0);                                              \
  return s->elements[--s->size];                                    \
}                                                                   \
                                                                    \
CIG_INLINED T stack_##T##_peek(T##_stack_t *s, size_t offset) {     \
  assert(s->size > 0);                                              \
  return s->elements[s->size-1-offset];                             \
}                                                                   \
                                                                    \
CIG_INLINED T* stack_##T##__pop(T##_stack_t *s) {                   \
  assert(s->size > 0);                                              \
  return &s->elements[--s->size];                                   \
}                                                                   \
                                                                    \
CIG_INLINED T* stack_##T##__peek(T##_stack_t *s, size_t offset) {   \
  assert(s->size > 0);                                              \
  return &s->elements[s->size-1-offset];                            \
}

#define INIT_STACK(T) (T##_stack_t) { \
  .capacity = STACK_CAPACITY_##T,     \
  .clear = &stack_##T##_clear,        \
  .push = &stack_##T##_push,          \
  .pop = &stack_##T##_pop,            \
  .peek = &stack_##T##_peek,          \
  ._pop = &stack_##T##__pop,          \
  ._peek = &stack_##T##__peek         \
}

#endif
