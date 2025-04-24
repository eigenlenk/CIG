#ifndef CIG_LIMITS_INCLUDED
#define CIG_LIMITS_INCLUDED

/* Maximum number of nested layout elements during the layout pass */
#define CIG_NESTED_ELEMENTS_MAX 32

/* Size of the internal scroll state pool; number of scrollable elements on
   screen at once. You can also provide this state from your application layer.
   Internal storage is just for convenience */
#define CIG_SCROLLABLE_ELEMENTS_MAX 32

/* Size of the internal widget state pool; number of stateful elements on
   screen at once. You can also provide this state from your application layer.
   Internal storage is just for convenience */
#define CIG_STATES_MAX 32

/* Size of the memory arena for each state */
#define CIG_STATE_MEM_ARENA_BYTES 512

/* How large is the buffer stack. Generally not too deeply nested? */
#define CIG_BUFFERS_MAX 4

/* Maximum number of clip regions you can push within a buffer. Every clip
   region pushed forms a union with the last, so there shouldn't be too many? */
#define CIG_BUFFER_CLIP_REGIONS_MAX 8


#ifdef CIG_TEXT_INCLUDED

/* Label is a single line of text and a span is basically a word
   that can have its properties overriden */
#define CIG_LABEL_SPANS_MAX 16

#define CIG_LABEL_PRINTF_BUF_LENGTH 512

#endif


#endif
