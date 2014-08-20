#include <pebble.h>
#include "uber.h"

int main(void) {
	uber_init();
	app_event_loop();
	uber_deinit();
}
