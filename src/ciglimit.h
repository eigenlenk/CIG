#ifndef CIG_LIMITS_INCLUDED
#define CIG_LIMITS_INCLUDED

/*  Maximum number of layout elements */
#define CIG_ELEMENTS_MAX 4096

/*  Maximum number of nested layout elements during the layout pass */
#define CIG_NESTED_ELEMENTS_MAX 32

/*  Size of the internal scroll state pool; number of scrollable elements on
    screen at once. You can also provide this state from your application layer.
    Internal pool is just for convenience */
#define CIG_SCROLLABLE_ELEMENTS_MAX 32

/*  Size of the internal widget state pool; number of stateful elements (+ labels) on
    screen at once. You can also provide this state from your application layer.
    Internal pool is just for convenience */
#define CIG_STATES_MAX 1024

/*  Size of the memory arena for each state (4KB) */
#define CIG_STATE_MEM_ARENA_BYTES 4096

/*  How large is the buffer stack. Generally not too deeply nested? */
#define CIG_BUFFERS_MAX 4

/*  Maximum number of clip regions you can push within a buffer. Every clip
    region pushed forms a union with the last, so there shouldn't be too many? */
#define CIG_BUFFER_CLIP_REGIONS_MAX 8

#endif
