#include <pebble.h>
#include "uber.h"
#include "libs/pebble-assist.h"
#include "generated/appinfo.h"
#include "generated/keys.h"
#include "appmessage.h"
#include "windows/products.h"

static void free_products();
static void timer_callback(void *data);
static AppTimer *timer = NULL;

typedef struct {
	uint32_t id;
	GRect bounds;
} ResourceImages;

ResourceImages resource_images[] = {
	{ RESOURCE_ID_IMAGE_UBERBLACK,     { { 1, 12 }, { 20, 6 } } },
	{ RESOURCE_ID_IMAGE_UBERX,         { { 1, 12 }, { 20, 7 } } },
	{ RESOURCE_ID_IMAGE_UBERXL,        { { 1, 12 }, { 20, 7 } } },
	{ RESOURCE_ID_IMAGE_UBERTAXI,      { { 1, 12 }, { 20, 7 } } },
	{ RESOURCE_ID_IMAGE_UBERBLACKTAXI, { { 1, 12 }, { 20, 7 } } },
	{ RESOURCE_ID_IMAGE_UBERLUX,       { { 1, 12 }, { 20, 7 } } },
	{ RESOURCE_ID_IMAGE_UBERSUV,       { { 1, 11 }, { 20, 8 } } },
	{ RESOURCE_ID_IMAGE_UBERT,         { { 4,  8 }, { 14, 13 } } },
};

Product* products = NULL;
Location* locations = NULL;
char* error = NULL;
uint8_t num_products = 0;
uint8_t num_locations = 0;
uint8_t selected_product = 0;
uint8_t selected_location = 0;

void uber_init(void) {
	appmessage_init();

	timer = app_timer_register(1000, timer_callback, NULL);

	products_init();
}

void uber_deinit(void) {
	free_safe(error);
	free_safe(locations);
	free_products();
	products_deinit();
}

void uber_in_received_handler(DictionaryIterator *iter) {
	if (!dict_find(iter, APP_KEY_TYPE)) return;
	free_safe(error);
	switch (dict_find(iter, APP_KEY_TYPE)->value->uint8) {
		case KEY_TYPE_ERROR: {
			error = malloc(dict_find(iter, APP_KEY_NAME)->length);
			if (error)
				strncpy(error, dict_find(iter, APP_KEY_NAME)->value->cstring, dict_find(iter, APP_KEY_NAME)->length);
			reload_data_and_mark_dirty();
			break;
		}
		case KEY_TYPE_PRODUCT:
			switch (dict_find(iter, APP_KEY_METHOD)->value->uint8) {
				case KEY_METHOD_SIZE:
					free_products();
					num_products = dict_find(iter, APP_KEY_INDEX)->value->uint8;
					products = malloc(sizeof(Product) * num_products);
					if (products == NULL) num_products = 0;
					break;
				case KEY_METHOD_DATA: {
					if (num_products == 0) break;
					uint8_t index = dict_find(iter, APP_KEY_INDEX)->value->uint8;
					uint32_t resource = dict_find(iter, APP_KEY_RESOURCE)->value->uint32;
					Product *product = &products[index];
					product->index = index;
					product->image = gbitmap_create_with_resource(resource_images[resource].id);
					product->image_bounds = resource_images[resource].bounds;
					strncpy(product->name, dict_find(iter, APP_KEY_NAME)->value->cstring, sizeof(product->name) - 1);
					strncpy(product->estimate, dict_find(iter, APP_KEY_ESTIMATE)->value->cstring, sizeof(product->estimate) - 1);
					strncpy(product->surge, dict_find(iter, APP_KEY_SURGE)->value->cstring, sizeof(product->surge) - 1);
					LOG("product: %d '%s' '%s' '%s'", product->index, product->name, product->estimate, product->surge);
					reload_data_and_mark_dirty();
					break;
				}
			}
			break;
		case KEY_TYPE_LOCATION:
			switch (dict_find(iter, APP_KEY_METHOD)->value->uint8) {
				case KEY_METHOD_SIZE:
					free_safe(locations);
					num_locations = dict_find(iter, APP_KEY_INDEX)->value->uint8;
					locations = malloc(sizeof(Location) * num_locations);
					if (locations == NULL) num_locations = 0;
					break;
				case KEY_METHOD_DATA: {
					if (num_locations == 0) break;
					uint8_t index = dict_find(iter, APP_KEY_INDEX)->value->uint8;
					Location *location = &locations[index];
					location->index = index;
					strncpy(location->name, dict_find(iter, APP_KEY_NAME)->value->cstring, sizeof(location->name) - 1);
					strncpy(location->estimate, dict_find(iter, APP_KEY_ESTIMATE)->value->cstring, sizeof(location->estimate) - 1);
					LOG("location: %d '%s' '%s'", location->index, location->name, location->estimate);
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
	num_locations = 0;
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	dict_write_uint8(iter, APP_KEY_METHOD, KEY_METHOD_REFRESH);
	dict_write_end(iter);
	app_message_outbox_send();
	reload_data_and_mark_dirty();
}

void uber_request_locations() {
	num_locations = 0;
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	dict_write_uint8(iter, APP_KEY_METHOD, KEY_METHOD_REQUESTLOCATIONS);
	dict_write_uint8(iter, APP_KEY_INDEX, selected_product);
	dict_write_end(iter);
	app_message_outbox_send();
	reload_data_and_mark_dirty();
}

void uber_request_price() {
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	dict_write_uint8(iter, APP_KEY_METHOD, KEY_METHOD_REQUESTPRICE);
	dict_write_uint8(iter, APP_KEY_INDEX, selected_product);
	dict_write_end(iter);
	app_message_outbox_send();
	reload_data_and_mark_dirty();
}

void reload_data_and_mark_dirty() {
	products_reload_data_and_mark_dirty();
}

Product* product_get(uint8_t index) {
	if (index < num_products)
		return &products[index];
	return NULL;
}

Product* product() {
	return &products[selected_product];
}

Location* location() {
	return &locations[selected_location];
}

static void free_products() {
	if (products != NULL) {
		for (int i = 0; i < num_products; i++) {
			gbitmap_destroy_safe(products[i].image);
		}
		free_safe(products);
	}
}

static void timer_callback(void *data) {
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	dict_write_uint8(iter, APP_KEY_METHOD, KEY_METHOD_READY);
	dict_write_end(iter);
	app_message_outbox_send();
}
