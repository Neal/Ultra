#include <pebble.h>
#include "locations.h"
#include "libs/pebble-assist.h"
#include "generated/appinfo.h"
#include "generated/keys.h"
#include "windows/win-locations.h"

static Location *locations = NULL;

static uint8_t num_locations = 0;
static uint8_t current_location = 0;

static char *error = NULL;

void locations_init(void) {
	win_locations_init();
}

void locations_deinit(void) {
	free_safe(error);
	free_safe(locations);
	win_locations_deinit();
}

void locations_in_received_handler(DictionaryIterator *iter) {
	Tuple *tuple = dict_find(iter, APP_KEY_METHOD);
	if (!tuple) return;
	free_safe(error);
	switch (tuple->value->uint8) {
		case KEY_METHOD_ERROR: {
			tuple = dict_find(iter, APP_KEY_NAME);
			if (!tuple) break;
			error = malloc(tuple->length);
			if (error)
				strncpy(error, tuple->value->cstring, tuple->length);
			locations_reload_data_and_mark_dirty();
			break;
		}
		case KEY_METHOD_SIZE:
			free_safe(locations);
			tuple = dict_find(iter, APP_KEY_INDEX);
			if (!tuple) break;
			num_locations = tuple->value->uint8;
			locations = malloc(sizeof(Location) * num_locations);
			if (locations == NULL) num_locations = 0;
			break;
		case KEY_METHOD_DATA: {
			if (!locations_count()) break;
			tuple = dict_find(iter, APP_KEY_INDEX);
			if (!tuple) break;
			uint8_t index = tuple->value->uint8;
			Location *location = locations_get(index);
			location->index = index;
			tuple = dict_find(iter, APP_KEY_NAME);
			if (tuple) {
				strncpy(location->name, tuple->value->cstring, sizeof(location->name) - 1);
			}
			tuple = dict_find(iter, APP_KEY_ESTIMATE);
			if (tuple) {
				strncpy(location->estimate, tuple->value->cstring, sizeof(location->estimate) - 1);
			}
			tuple = dict_find(iter, APP_KEY_DURATION);
			if (tuple) {
				strncpy(location->duration, tuple->value->cstring, sizeof(location->duration) - 1);
			}
			tuple = dict_find(iter, APP_KEY_DISTANCE);
			if (tuple) {
				strncpy(location->distance, tuple->value->cstring, sizeof(location->distance) - 1);
			}
			LOG("location: %d '%s' '%s' '%s' '%s'", location->index, location->name, location->estimate, location->duration, location->distance);
			locations_reload_data_and_mark_dirty();
			break;
		}
	}
}

void locations_reload_data_and_mark_dirty() {
	win_locations_reload_data_and_mark_dirty();
}

uint8_t locations_count() {
	return num_locations;
}

void locations_count_set(uint8_t count) {
	num_locations = count;
}

char* locations_get_error() {
	return (error == NULL && !locations_count()) ? "Loading..." : error;
}

Location* locations_get(uint8_t index) {
	return (index < locations_count()) ? &locations[index] : NULL;
}

Location* locations_get_current() {
	return &locations[current_location];
}

void locations_set_current(uint8_t index) {
	current_location = index;
}
