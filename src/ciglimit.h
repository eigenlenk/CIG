#ifndef IM_LIMITS_H
#define IM_LIMITS_H

/* Maximum number of nested layout elements during the layout pass */
#define CIG_NESTED_ELEMENTS_MAX 32

/* Size of the internal scroll state pool; number of scrollable elements on
   screen at once. You can also provide this state from your application layer.
   Internal storage is just for convenience */
#define CIG_SCROLLABLE_ELEMENTS_MAX 32

/* Size of the internal widget state pool; number of stateful elements on
   screen at once. You can also provide this state from your application layer.
   Internal storage is just for convenience */
#define CIG_STATES_MAX 64

/* How many bytes does each state get. You can bend and mould this to your own data structures */
#define CIG_STATE_MEM_SIZE_BYTES 512

/* How large is the buffer stack. Generally not too deeply nested */
#define CIG_BUFFERS_MAX 4

/* Maximum number of clip regions you can push within a buffer */
#define CIG_BUFFER_CLIP_REGIONS_MAX 8

#endif
