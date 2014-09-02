#include <pebble.h>
#include "uber.h"
#include "libs/pebble-assist.h"
#include "generated/appinfo.h"
#include "generated/keys.h"
#include "appmessage.h"
#include "product.h"
#include "location.h"

static void timer_callback(void *data);
static AppTimer *timer = NULL;

static char* error = NULL;

void uber_init(void) {
	appmessage_init();

	timer = app_timer_register(1000, timer_callback, NULL);

	product_init();
	location_init();
}

void uber_deinit(void) {
	free_safe(error);
	location_deinit();
	product_deinit();
}

void uber_in_received_handler(DictionaryIterator *iter) {
	if (!dict_find(iter, APP_KEY_TYPE)) return;
	free_safe(error);
	switch (dict_find(iter, APP_KEY_TYPE)->value->uint8) {
		case KEY_TYPE_ERROR: {
			error = malloc(dict_find(iter, APP_KEY_NAME)->length);
			if (error)
				strncpy(error, dict_find(iter, APP_KEY_NAME)->value->cstring, dict_find(iter, APP_KEY_NAME)->length);
			uber_reload_data_and_mark_dirty();
			break;
		}
		case KEY_TYPE_PRODUCT:
			product_in_received_handler(iter);
			break;
		case KEY_TYPE_LOCATION:
			location_in_received_handler(iter);
			break;
	}
}

void uber_out_failed_handler(DictionaryIterator *failed, AppMessageResult reason) {
	free_safe(error);
	error = malloc(sizeof(char) * 56);
	if (error)
		strncpy(error, "Phone unreachable! Make sure the Pebble app is running.", 55);
	uber_reload_data_and_mark_dirty();
}

void uber_request_products() {
	product_count_set(0);
	location_count_set(0);
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	dict_write_uint8(iter, APP_KEY_METHOD, KEY_METHOD_REQUESTPRODUCTS);
	dict_write_end(iter);
	app_message_outbox_send();
	uber_reload_data_and_mark_dirty();
}

void uber_request_locations() {
	location_count_set(0);
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	dict_write_uint8(iter, APP_KEY_METHOD, KEY_METHOD_REQUESTLOCATIONS);
	dict_write_uint8(iter, APP_KEY_INDEX, product_get_current_index());
	dict_write_end(iter);
	app_message_outbox_send();
	uber_reload_data_and_mark_dirty();
}

void uber_request_price() {
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	dict_write_uint8(iter, APP_KEY_METHOD, KEY_METHOD_REQUESTPRICE);
	dict_write_uint8(iter, APP_KEY_INDEX, product_get_current_index());
	dict_write_end(iter);
	app_message_outbox_send();
	uber_reload_data_and_mark_dirty();
}

void uber_reload_data_and_mark_dirty() {
	product_reload_data_and_mark_dirty();
	location_reload_data_and_mark_dirty();
}

char* uber_get_error() {
	return &error[0];
}

static void timer_callback(void *data) {
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	dict_write_uint8(iter, APP_KEY_METHOD, KEY_METHOD_READY);
	dict_write_end(iter);
	app_message_outbox_send();
}
