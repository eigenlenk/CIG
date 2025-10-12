#ifndef CIG_TYPE_GAP_BUFFER_T_INCLUDED
#define CIG_TYPE_GAP_BUFFER_T_INCLUDED

#include "cigmac.h"
#include <stddef.h>
#include <string.h>

#define GAPHEAD 0
#define GAPCURPOS -1
#define GAPTAIL -2

typedef struct {
  size_t size;
  // size_t element_size;
  struct {
    size_t start;
    size_t size;
  } gap;
  char buffer[];
} gap_buffer_char;

CIG_INLINED void
gap_buffer_char_new(gap_buffer_char **ptr, size_t position, size_t init_size)
{
  *ptr = (gap_buffer_char*)malloc(sizeof(gap_buffer_char) + sizeof(char) * init_size);
  // (*ptr)->element_size = sizeof(char);
  (*ptr)->size = init_size;
  (*ptr)->gap.start = position;
  (*ptr)->gap.size = init_size;
}

CIG_INLINED void
gap_buffer_char_place_gap(gap_buffer_char **ptr, size_t position)
{
  if (position == GAPCURPOS) {
    return;
  } else if (position == GAPTAIL) {
    position = (*ptr)->size - (*ptr)->gap.size;
  }

  if ((*ptr)->gap.start == position) {
    return;
  } else if (position < (*ptr)->gap.start) {
    const size_t num = ((*ptr)->gap.start - position);

    memmove(
      (*ptr)->buffer + (*ptr)->gap.start + (*ptr)->gap.size - num,
      (*ptr)->buffer + position,
      num
    );
  } else {
    const size_t num = (position - (*ptr)->gap.start);

    memmove(
      (*ptr)->buffer + (*ptr)->gap.start,
      (*ptr)->buffer + (*ptr)->gap.start + (*ptr)->gap.size,
      num
    );
  }

  (*ptr)->gap.start = position;
}

CIG_INLINED void
gap_buffer_char_extend_gap(gap_buffer_char **ptr, size_t size)
{
  if (size <= (*ptr)->gap.size) {
    return;
  }

  const size_t new_size = (*ptr)->size * 2;
  const size_t post_gap = (*ptr)->size - ((*ptr)->gap.start + (*ptr)->gap.size);

  *ptr = (gap_buffer_char*)realloc(*ptr, sizeof(gap_buffer_char) + sizeof(char) * new_size);

  memmove(
    (*ptr)->buffer + (*ptr)->gap.start + (*ptr)->gap.size + (new_size - (*ptr)->size),
    (*ptr)->buffer + (*ptr)->gap.start + (*ptr)->gap.size,
    post_gap
  );

  (*ptr)->gap.size += (new_size - (*ptr)->size);
  (*ptr)->size = new_size;
}

CIG_INLINED void
gap_buffer_char_insert(gap_buffer_char **ptr, ptrdiff_t position, size_t count, const char buffer[])
{
  gap_buffer_char_place_gap(ptr, position);
  gap_buffer_char_extend_gap(ptr, count);

  memcpy((*ptr)->buffer + (*ptr)->gap.start, buffer, count);

  (*ptr)->gap.start += count;
  (*ptr)->gap.size -= count;
}

CIG_INLINED void
gap_buffer_char_delete(gap_buffer_char **ptr, ptrdiff_t position, size_t range)
{
  gap_buffer_char_place_gap(ptr, position);
  (*ptr)->gap.size += range;
}

CIG_INLINED void
gap_buffer_char_replace(gap_buffer_char **ptr, ptrdiff_t position, size_t range, size_t count, const char buffer[])
{
  gap_buffer_char_delete(ptr, position, range);
  gap_buffer_char_insert(ptr, position, count, buffer);
}

CIG_INLINED void
gap_buffer_char_free(gap_buffer_char **ptr)
{
  if (*ptr) {
    free(*ptr);
    *ptr = NULL;
  }
}

#endif
