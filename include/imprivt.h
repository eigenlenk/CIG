#ifndef IM_PRIVATE_TYPES_H
#define IM_PRIVATE_TYPES_H

#include "types/vec2.h"

DECLARE_STACK(frame_t);

typedef struct {
	im_buffer_ref buffer;
	vec2 origin;
	STACK(frame_t) clip_frames;
} im_buffer_element_t;

typedef struct {
	IMGUIID id;
	enum {
		INACTIVE = 0,
		ACTIVATED,
		ACTIVE
	} activation_state;
	unsigned int last_tick;
	union {
		unsigned char bytes[IM_STATE_MEM_SIZE];
	} data;
} im_state_t;

typedef struct {
  IMGUIID id;
	unsigned int last_tick;
  im_scroll_state_t value;
} im_scroll_state_element_t;

#endif
