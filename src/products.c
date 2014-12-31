#include <pebble.h>
#include "products.h"
#include "libs/pebble-assist.h"
#include "generated/appinfo.h"
#include "generated/keys.h"
#include "windows/win-products.h"

#define NUM_RESOURCE_IMAGES sizeof(resource_images) / sizeof(resource_images[0])

ResourceImage resource_images[] = {
	{ RESOURCE_ID_IMAGE_UBERBLACK,       NULL, { { 1, 12 }, { 20, 6 } } },
	{ RESOURCE_ID_IMAGE_UBERPLUS,        NULL, { { 1, 12 }, { 20, 6 } } },
	{ RESOURCE_ID_IMAGE_UBERX,           NULL, { { 1, 12 }, { 20, 7 } } },
	{ RESOURCE_ID_IMAGE_UBERXL,          NULL, { { 1, 12 }, { 20, 7 } } },
	{ RESOURCE_ID_IMAGE_UBERTAXI,        NULL, { { 1, 12 }, { 20, 7 } } },
	{ RESOURCE_ID_IMAGE_UBERBLACKTAXI,   NULL, { { 1, 12 }, { 20, 7 } } },
	{ RESOURCE_ID_IMAGE_UBERLUX,         NULL, { { 1, 12 }, { 20, 7 } } },
	{ RESOURCE_ID_IMAGE_UBERSUV,         NULL, { { 1, 11 }, { 20, 8 } } },
	{ RESOURCE_ID_IMAGE_UBERT,           NULL, { { 4,  8 }, { 14, 13 } } },
};

static Product *products = NULL;

static uint8_t num_products = 0;
static uint8_t current_product = 0;

static char *error = NULL;

void products_init(void) {
	for (uint i = 0; i < NUM_RESOURCE_IMAGES; i++) {
		resource_images[i].image = gbitmap_create_with_resource(resource_images[i].id);
	}
	win_products_init();
}

void products_deinit(void) {
	free_safe(error);
	free_safe(products);
	for (uint i = 0; i < NUM_RESOURCE_IMAGES; i++) {
		gbitmap_destroy_safe(resource_images[i].image);
	}
	win_products_deinit();
}

void products_in_received_handler(DictionaryIterator *iter) {
	Tuple *tuple = dict_find(iter, APP_KEY_METHOD);
	if (!tuple) return;
	free_safe(error);
	switch (tuple->value->uint8) {
		case KEY_METHOD_ERROR: {
			tuple = dict_find(iter, APP_KEY_NAME);
			if (!tuple) break;
			error = malloc(tuple->length);
			if (error)
				strncpy(error, tuple->value->cstring, tuple->length);
			products_reload_data_and_mark_dirty();
			break;
		}
		case KEY_METHOD_SIZE:
			free_safe(products);
			tuple = dict_find(iter, APP_KEY_INDEX);
			if (!tuple) break;
			num_products = tuple->value->uint8;
			products = malloc(sizeof(Product) * num_products);
			if (products == NULL) num_products = 0;
			break;
		case KEY_METHOD_DATA: {
			if (!products_count()) break;
			tuple = dict_find(iter, APP_KEY_INDEX);
			if (!tuple) break;
			uint8_t index = tuple->value->uint8;
			Product *product = products_get(index);
			product->index = index;
			tuple = dict_find(iter, APP_KEY_RESOURCE);
			if (tuple) {
				product->resource = resource_images[tuple->value->uint32];
			}
			tuple = dict_find(iter, APP_KEY_NAME);
			if (tuple) {
				strncpy(product->name, tuple->value->cstring, sizeof(product->name) - 1);
			}
			tuple = dict_find(iter, APP_KEY_ESTIMATE);
			if (tuple) {
				strncpy(product->estimate, tuple->value->cstring, sizeof(product->estimate) - 1);
			}
			tuple = dict_find(iter, APP_KEY_SURGE);
			if (tuple) {
				strncpy(product->surge, tuple->value->cstring, sizeof(product->surge) - 1);
			}
			LOG("product: %d '%s' '%s' '%s'", product->index, product->name, product->estimate, product->surge);
			products_reload_data_and_mark_dirty();
			break;
		}
	}
}

void products_reload_data_and_mark_dirty() {
	win_products_reload_data_and_mark_dirty();
}

uint8_t products_count() {
	return num_products;
}

void products_count_set(uint8_t count) {
	num_products = count;
}

char* products_get_error() {
	return (error == NULL && !products_count()) ? "Loading..." : error;
}

Product* products_get(uint8_t index) {
	return (index < products_count()) ? &products[index] : NULL;
}

Product* products_get_current() {
	return &products[current_product];
}

uint8_t products_get_current_index() {
	return current_product;
}

void products_set_current(uint8_t index) {
	current_product = index;
}
