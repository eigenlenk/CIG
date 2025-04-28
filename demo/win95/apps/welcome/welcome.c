#include "welcome.h"

typedef struct {

} window_data_t;

static window_proc_result_t process_main_window(window_t *this) {
	window_data_t *window_data = (window_data_t*)this->data;
	window_proc_result_t result = 0;


	return result;
}

application_t welcome_app() {
	return (application_t) {
		.windows = {
			(window_t) {
				.proc = &process_main_window,
				.data = malloc(sizeof(window_data_t)),
				.title = "Welcome",
				.icon = -1
			}
		},
		.data = NULL
	};
}