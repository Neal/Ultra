#include <pebble.h>
#include "uber.h"
#include "libs/pebble-assist.h"
#include "common.h"
#include "appmessage.h"
#include "windows/products.h"

static void timer_callback(void *data);
static AppTimer *timer = NULL;

Product* products = NULL;
char* error = NULL;
uint8_t num_products = 0;
uint8_t selected_product = 0;

void uber_init(void) {
	appmessage_init();

	timer = app_timer_register(1000, timer_callback, NULL);

	products_init();
}

void uber_deinit(void) {
	free_safe(error);
	free_safe(products);
	products_deinit();
}

void uber_in_received_handler(DictionaryIterator *iter) {
	if (!dict_find(iter, KEY_TYPE)) return;
	free_safe(error);
	switch (dict_find(iter, KEY_TYPE)->value->uint8) {
		case KEY_TYPE_ERROR: {
			error = malloc(dict_find(iter, KEY_NAME)->length);
			if (error)
				strncpy(error, dict_find(iter, KEY_NAME)->value->cstring, dict_find(iter, KEY_NAME)->length - 1);
			reload_data_and_mark_dirty();
			break;
		}
		case KEY_TYPE_PRODUCT:
			switch (dict_find(iter, KEY_METHOD)->value->uint8) {
				case KEY_METHOD_SIZE:
					free_safe(products);
					num_products = dict_find(iter, KEY_INDEX)->value->uint8;
					products = malloc(sizeof(Product) * num_products);
					if (products == NULL) num_products = 0;
					break;
				case KEY_METHOD_DATA: {
					if (num_products == 0) break;
					uint8_t index = dict_find(iter, KEY_INDEX)->value->uint8;
					Product *product = &products[index];
					product->index = index;
					strncpy(product->name, dict_find(iter, KEY_NAME)->value->cstring, sizeof(product->name) - 1);
					strncpy(product->estimate, dict_find(iter, KEY_ESTIMATE)->value->cstring, sizeof(product->estimate) - 1);
					LOG("product: %d '%s' '%s'", product->index, product->name, product->estimate);
					reload_data_and_mark_dirty();
					break;
				}
			}
			break;
	}
}

void uber_out_sent_handler(DictionaryIterator *sent) {
}

void uber_out_failed_handler(DictionaryIterator *failed, AppMessageResult reason) {
	free_safe(error);
	error = malloc(sizeof(char) * 56);
	if (error)
		strncpy(error, "Phone unreachable! Make sure the Pebble app is running.", 55);
	reload_data_and_mark_dirty();
}

void uber_refresh() {
	num_products = 0;
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	dict_write_uint8(iter, KEY_METHOD, KEY_METHOD_REFRESH);
	dict_write_end(iter);
	app_message_outbox_send();
	reload_data_and_mark_dirty();
}

void reload_data_and_mark_dirty() {
	products_reload_data_and_mark_dirty();
}

Product* product() {
	return &products[selected_product];
}

static void timer_callback(void *data) {
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	dict_write_uint8(iter, KEY_METHOD, KEY_METHOD_READY);
	dict_write_end(iter);
	app_message_outbox_send();
}
