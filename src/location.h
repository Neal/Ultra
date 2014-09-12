#pragma once

#include <pebble.h>

typedef struct {
	uint8_t index;
	char name[12];
	char estimate[12];
} Location;

void location_init(void);
void location_deinit(void);
void location_in_received_handler(DictionaryIterator *iter);
void location_reload_data_and_mark_dirty();
uint8_t location_count();
void location_count_set(uint8_t count);
char* location_get_error();
Location* location_get(uint8_t index);
Location* location_get_current();
void location_set_current(uint8_t index);
