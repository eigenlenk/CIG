#ifndef STACK_H
#define STACK_H

#include <stddef.h>
#include <stdbool.h>

#define STACK_CAPACITY 32

#define DECLARE_STACK(T) \
	typedef struct { \
		T elements[STACK_CAPACITY]; \
		size_t size; \
	} stack_##T;

#define STACK(T) stack_##T
#define STACK_INIT(S) (S)->size = 0;
#define STACK_PUSH(S, V) (S)->elements[(S)->size++] = V;
#define STACK_POP(S) (S)->elements[--(S)->size]
#define STACK_TOP(S) (S)->elements[(S)->size-1]
#define STACK_POP_NORETURN(S) (S)->size--
#define STACK_IS_EMPTY(S) (S)->size == 0
#define STACK_IS_FULL(S) (S)->size == STACK_CAPACITY
#define STACK_SIZE(S) (S)->size
#define STACK_AT(S, OFFSET) (S)->elements[(S)->size-OFFSET-1];
#define STACK_PRINT(S, F) { \
	printf("Stack %p:\n", S); \
	if (!(S)->size) { \
		printf("\t(Empty)\n"); \
	} else { \
		for (int i = (int)(S)->size - 1; i >= 0; --i) { \
			printf("\t[%d] ", i); \
			printf(F, (S)->elements[i]); \
			printf("\n"); \
		} \
	} \
}

// Declare stacks of basic types

DECLARE_STACK(int)
DECLARE_STACK(double)
DECLARE_STACK(bool)

typedef char* char_ptr;
DECLARE_STACK(char_ptr)

#endif
