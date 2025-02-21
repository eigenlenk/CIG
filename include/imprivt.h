#ifndef IM_PRIVATE_TYPES_H
#define IM_PRIVATE_TYPES_H

#include "types/vec2.h"

typedef struct {
	IMGUIID id;
	imgui_widget_t type;
	int internal_record;
	im_buffer_ref buffer;
	enum {
		INACTIVE = 0,
		ACTIVATED,
		ACTIVE
	} activation_state;
	unsigned int last_tick;
	union {
		unsigned char bytes[IM_STATE_MEM_SIZE];
		/*im_menubar_dt menubar;
		im_menu_dt menu;
		im_window_dt window;
		im_multiline_text_info_t text;
		im_input_dt input;
		im_scroller_dt scroller;
		im_slider_dt slider;
		im_dynamic_list_dt dynamic_list;*/
	} data;
} im_state_t;

#endif
