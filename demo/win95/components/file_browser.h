#ifndef CIG_WIN95_DEMO_FILE_BROWSER_INCLUDED
#define CIG_WIN95_DEMO_FILE_BROWSER_INCLUDED

#include "cig.h"
#include "system/resources.h"

bool begin_file_browser(cig_r, int, color_id_t, bool, int *);
bool file_item(image_id_t, const char*);
void end_file_browser();

#endif
