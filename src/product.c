#include <pebble.h>
#include "product.h"
#include "libs/pebble-assist.h"
#include "generated/appinfo.h"
#include "generated/keys.h"
#include "windows/products.h"

#define NUM_RESOURCE_IMAGES sizeof(resource_images) / sizeof(resource_images[0])

ResourceImage resource_images[] = {
	{ RESOURCE_ID_IMAGE_UBERBLACK,       NULL, { { 1, 12 }, { 20, 6 } } },
	{ RESOURCE_ID_IMAGE_UBERX,           NULL, { { 1, 12 }, { 20, 7 } } },
	{ RESOURCE_ID_IMAGE_UBERXL,          NULL, { { 1, 12 }, { 20, 7 } } },
	{ RESOURCE_ID_IMAGE_UBERTAXI,        NULL, { { 1, 12 }, { 20, 7 } } },
	{ RESOURCE_ID_IMAGE_UBERBLACKTAXI,   NULL, { { 1, 12 }, { 20, 7 } } },
	{ RESOURCE_ID_IMAGE_UBERLUX,         NULL, { { 1, 12 }, { 20, 7 } } },
	{ RESOURCE_ID_IMAGE_UBERSUV,         NULL, { { 1, 11 }, { 20, 8 } } },
	{ RESOURCE_ID_IMAGE_UBERT,           NULL, { { 4,  8 }, { 14, 13 } } },
};

static Product* products = NULL;

static uint8_t num_products = 0;
static uint8_t current_product = 0;

static char* error = NULL;

void product_init(void) {
	for (uint i = 0; i < NUM_RESOURCE_IMAGES; i++) {
		resource_images[i].image = gbitmap_create_with_resource(resource_images[i].id);
	}
	products_init();
}

void product_deinit(void) {
	free_safe(error);
	free_safe(products);
	for (uint i = 0; i < NUM_RESOURCE_IMAGES; i++) {
		gbitmap_destroy_safe(resource_images[i].image);
	}
	products_deinit();
}

void product_in_received_handler(DictionaryIterator *iter) {
	if (!dict_find(iter, APP_KEY_METHOD)) return;
	free_safe(error);
	switch (dict_find(iter, APP_KEY_METHOD)->value->uint8) {
		case KEY_METHOD_ERROR: {
			error = malloc(dict_find(iter, APP_KEY_NAME)->length);
			if (error)
				strncpy(error, dict_find(iter, APP_KEY_NAME)->value->cstring, dict_find(iter, APP_KEY_NAME)->length);
			product_reload_data_and_mark_dirty();
			break;
		}
		case KEY_METHOD_SIZE:
			free_safe(products);
			num_products = dict_find(iter, APP_KEY_INDEX)->value->uint8;
			products = malloc(sizeof(Product) * num_products);
			if (products == NULL) num_products = 0;
			break;
		case KEY_METHOD_DATA: {
			if (!product_count()) break;
			uint8_t index = dict_find(iter, APP_KEY_INDEX)->value->uint8;
			uint32_t resource = dict_find(iter, APP_KEY_RESOURCE)->value->uint32;
			Product *product = product_get(index);
			product->index = index;
			product->resource = resource_images[resource];
			strncpy(product->name, dict_find(iter, APP_KEY_NAME)->value->cstring, sizeof(product->name) - 1);
			strncpy(product->estimate, dict_find(iter, APP_KEY_ESTIMATE)->value->cstring, sizeof(product->estimate) - 1);
			strncpy(product->surge, dict_find(iter, APP_KEY_SURGE)->value->cstring, sizeof(product->surge) - 1);
			LOG("product: %d '%s' '%s' '%s'", product->index, product->name, product->estimate, product->surge);
			product_reload_data_and_mark_dirty();
			break;
		}
	}
}

void product_reload_data_and_mark_dirty() {
	products_reload_data_and_mark_dirty();
}

uint8_t product_count() {
	return num_products;
}

void product_count_set(uint8_t count) {
	num_products = count;
}

char* product_get_error() {
	if (error == NULL && !product_count()) return "Loading...";
	return &error[0];
}

Product* product_get(uint8_t index) {
	if (index < product_count())
		return &products[index];
	return NULL;
}

Product* product_get_current() {
	return &products[current_product];
}

uint8_t product_get_current_index() {
	return current_product;
}

void product_set_current(uint8_t index) {
	current_product = index;
}
