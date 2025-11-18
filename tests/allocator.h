#ifndef CIG_TESTS_ALLOCATOR_INCLUDED
#define CIG_TESTS_ALLOCATOR_INCLUDED

#include "cigcore.h"

extern int alloc_count, realloc_count, free_count;

void set_up_test_allocator(cig_context *);

#endif
