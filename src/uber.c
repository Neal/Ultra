#include <pebble.h>
#include "uber.h"
#include "libs/pebble-assist.h"
#include "generated/appinfo.h"
#include "generated/keys.h"
#include "products.h"
#include "locations.h"

static void in_received_handler(DictionaryIterator *iter, void *context);
static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context);
static void timer_callback(void *data);

static AppTimer *timer = NULL;

static char *error = NULL;

void uber_init(void) {
	app_message_register_inbox_received(in_received_handler);
	app_message_register_outbox_failed(out_failed_handler);
	app_message_open_max();

	timer = app_timer_register(1000, timer_callback, NULL);

	products_init();
	locations_init();
}

void uber_deinit(void) {
	free_safe(error);
	locations_deinit();
	products_deinit();
}

void uber_request_products() {
	products_count_set(0);
	locations_count_set(0);
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	dict_write_uint8(iter, APP_KEY_METHOD, KEY_METHOD_REQUESTPRODUCTS);
	dict_write_end(iter);
	app_message_outbox_send();
	uber_reload_data_and_mark_dirty();
}

void uber_request_locations() {
	locations_count_set(0);
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	dict_write_uint8(iter, APP_KEY_METHOD, KEY_METHOD_REQUESTLOCATIONS);
	dict_write_uint8(iter, APP_KEY_INDEX, products_get_current_index());
	dict_write_end(iter);
	app_message_outbox_send();
	uber_reload_data_and_mark_dirty();
}

void uber_request_price() {
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	dict_write_uint8(iter, APP_KEY_METHOD, KEY_METHOD_REQUESTPRICE);
	dict_write_uint8(iter, APP_KEY_INDEX, products_get_current_index());
	dict_write_end(iter);
	app_message_outbox_send();
	uber_reload_data_and_mark_dirty();
}

void uber_reload_data_and_mark_dirty() {
	products_reload_data_and_mark_dirty();
	locations_reload_data_and_mark_dirty();
}

char* uber_get_error() {
	return error;
}

static void in_received_handler(DictionaryIterator *iter, void *context) {
	Tuple *tuple = dict_find(iter, APP_KEY_TYPE);
	if (!tuple) return;
	free_safe(error);
	switch (tuple->value->uint8) {
		case KEY_TYPE_ERROR: {
			tuple = dict_find(iter, APP_KEY_NAME);
			error = malloc(tuple->length);
			strncpy(error, tuple->value->cstring, tuple->length);
			uber_reload_data_and_mark_dirty();
			break;
		}
		case KEY_TYPE_PRODUCT:
			products_in_received_handler(iter);
			break;
		case KEY_TYPE_LOCATION:
			locations_in_received_handler(iter);
			break;
	}
}

static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
	free_safe(error);
	error = malloc(sizeof(char) * 56);
	strncpy(error, "Phone unreachable! Make sure the Pebble app is running.", 55);
	uber_reload_data_and_mark_dirty();
}

static void timer_callback(void *data) {
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	dict_write_uint8(iter, APP_KEY_METHOD, KEY_METHOD_READY);
	dict_write_end(iter);
	app_message_outbox_send();
}
