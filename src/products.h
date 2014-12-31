#pragma once

#include <pebble.h>

typedef struct {
	uint32_t id;
	GBitmap *image;
	GRect bounds;
} ResourceImage;

typedef struct {
	uint8_t index;
	char name[12];
	char estimate[8];
	char surge[24];
	ResourceImage resource;
} Product;

void products_init(void);
void products_deinit(void);
void products_in_received_handler(DictionaryIterator *iter);
void products_reload_data_and_mark_dirty();
uint8_t products_count();
void products_count_set(uint8_t count);
char* products_get_error();
Product* products_get(uint8_t index);
Product* products_get_current();
uint8_t products_get_current_index();
void products_set_current(uint8_t index);
