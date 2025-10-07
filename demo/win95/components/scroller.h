#ifndef CIG_WIN95_DEMO_SCROLL_VIEW_INCLUDED
#define CIG_WIN95_DEMO_SCROLL_VIEW_INCLUDED

#include "cig.h"

typedef enum CIG_PACKED {
  SCROLLER_DISPLAYED_X = CIG_BIT(0),
  SCROLLER_DISPLAYED_Y = CIG_BIT(1)
} scroller_results;

void
scroll_bar(cig_r, int32_t*, int32_t);

void
display_scrollbars(cig_scroll_state_t*, CIG_OPTIONAL(scroller_results*));

#endif
