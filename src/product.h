#pragma once

#include <pebble.h>

typedef struct {
	uint8_t index;
	char name[12];
	char estimate[8];
	char surge[24];
	GBitmap *image;
	GRect image_bounds;
} Product;

void product_init(void);
void product_deinit(void);
void product_in_received_handler(DictionaryIterator *iter);
void product_reload_data_and_mark_dirty();
uint8_t product_count();
void product_count_set(uint8_t count);
char* product_get_error();
Product* product_get(uint8_t index);
Product* product_get_current();
uint8_t product_get_current_index();
void product_set(uint8_t index);
