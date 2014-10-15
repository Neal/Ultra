#include <pebble.h>
#include "location.h"
#include "libs/pebble-assist.h"
#include "generated/appinfo.h"
#include "generated/keys.h"
#include "windows/locations.h"

static Location* locations = NULL;

static uint8_t num_locations = 0;
static uint8_t current_location = 0;

static char* error = NULL;

void location_init(void) {
	locations_init();
}

void location_deinit(void) {
	free_safe(error);
	free_safe(locations);
	locations_deinit();
}

void location_in_received_handler(DictionaryIterator *iter) {
	if (!dict_find(iter, APP_KEY_METHOD)) return;
	free_safe(error);
	switch (dict_find(iter, APP_KEY_METHOD)->value->uint8) {
		case KEY_METHOD_ERROR: {
			error = malloc(dict_find(iter, APP_KEY_NAME)->length);
			if (error)
				strncpy(error, dict_find(iter, APP_KEY_NAME)->value->cstring, dict_find(iter, APP_KEY_NAME)->length);
			location_reload_data_and_mark_dirty();
			break;
		}
		case KEY_METHOD_SIZE:
			free_safe(locations);
			num_locations = dict_find(iter, APP_KEY_INDEX)->value->uint8;
			locations = malloc(sizeof(Location) * num_locations);
			if (locations == NULL) num_locations = 0;
			break;
		case KEY_METHOD_DATA: {
			if (!location_count()) break;
			uint8_t index = dict_find(iter, APP_KEY_INDEX)->value->uint8;
			Location *location = location_get(index);
			location->index = index;
			strncpy(location->name, dict_find(iter, APP_KEY_NAME)->value->cstring, sizeof(location->name) - 1);
			strncpy(location->estimate, dict_find(iter, APP_KEY_ESTIMATE)->value->cstring, sizeof(location->estimate) - 1);
			strncpy(location->duration, dict_find(iter, APP_KEY_DURATION)->value->cstring, sizeof(location->duration) - 1);
			strncpy(location->distance, dict_find(iter, APP_KEY_DISTANCE)->value->cstring, sizeof(location->distance) - 1);
			LOG("location: %d '%s' '%s' '%s' '%s'", location->index, location->name, location->estimate, location->duration, location->distance);
			location_reload_data_and_mark_dirty();
			break;
		}
	}
}

void location_reload_data_and_mark_dirty() {
	locations_reload_data_and_mark_dirty();
}

uint8_t location_count() {
	return num_locations;
}

void location_count_set(uint8_t count) {
	num_locations = count;
}

char* location_get_error() {
	if (error == NULL && !location_count()) return "Loading...";
	return &error[0];
}

Location* location_get(uint8_t index) {
	if (index < location_count())
		return &locations[index];
	return NULL;
}

Location* location_get_current() {
	return &locations[current_location];
}

void location_set_current(uint8_t index) {
	current_location = index;
}
