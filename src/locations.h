#pragma once

#include <pebble.h>

typedef struct {
	uint8_t index;
	char name[12];
	char estimate[12];
	char duration[12];
	char distance[12];
} Location;

void locations_init(void);
void locations_deinit(void);
void locations_in_received_handler(DictionaryIterator *iter);
void locations_reload_data_and_mark_dirty();
uint8_t locations_count();
void locations_count_set(uint8_t count);
char* locations_get_error();
Location* locations_get(uint8_t index);
Location* locations_get_current();
void locations_set_current(uint8_t index);
