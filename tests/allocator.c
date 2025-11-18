#include "allocator.h"
#include "cigcore.h"

int alloc_count, realloc_count, free_count;

static void*
test_alloc(void *ud, size_t size, size_t align)
{
  alloc_count++;
  return malloc(size);
}

static void*
test_realloc(void *ud, void *ptr, size_t old_size, size_t new_size)
{
  void *new_bytes = realloc(ptr, new_size);
  assert(new_bytes);
  realloc_count++;
  return new_bytes;
}

static void
test_free(void *ud, void *ptr)
{
  assert(ptr);
  free_count++;
  free(ptr);
}

void
set_up_test_allocator(cig_context *context)
{
  alloc_count = 0;
  realloc_count = 0;
  free_count = 0;

  cig_set_allocator(context, (cig_allocator) {
    .alloc = test_alloc,
    .realloc = test_realloc,
    .free = test_free,
    .ud = NULL
  });
}
