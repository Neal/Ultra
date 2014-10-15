#include <pebble.h>
#include "products.h"
#include "libs/pebble-assist.h"
#include "uber.h"
#include "product.h"
#include "locations.h"

static uint16_t menu_get_num_rows_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context);
static int16_t menu_get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context);
static void menu_draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context);
static void menu_select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context);
static void menu_select_long_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context);

static Window *window = NULL;
static MenuLayer *menu_layer = NULL;
static GBitmap *surge = NULL;

void products_init(void) {
	window = window_create();

	menu_layer = menu_layer_create_fullscreen(window);
	menu_layer_set_callbacks(menu_layer, NULL, (MenuLayerCallbacks) {
		.get_num_rows = menu_get_num_rows_callback,
		.get_cell_height = menu_get_cell_height_callback,
		.draw_row = menu_draw_row_callback,
		.select_click = menu_select_callback,
		.select_long_click = menu_select_long_callback,
	});
	menu_layer_set_click_config_onto_window(menu_layer, window);
	menu_layer_add_to_window(menu_layer, window);

	surge = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SURGE);

	window_stack_push(window, true);
}

void products_deinit(void) {
	gbitmap_destroy_safe(surge);
	menu_layer_destroy_safe(menu_layer);
	window_destroy_safe(window);
}

void products_reload_data_and_mark_dirty(void) {
	menu_layer_reload_data_and_mark_dirty(menu_layer);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - //

static uint16_t menu_get_num_rows_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context) {
	return product_count() ? product_count() : 1;
}

static int16_t menu_get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
	if (uber_get_error()) {
		return graphics_text_layout_get_content_size(uber_get_error(), fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), GRect(4, 2, 136, 128), GTextOverflowModeFill, GTextAlignmentLeft).h + 12;
	} else if (product_get_error()) {
		return graphics_text_layout_get_content_size(product_get_error(), fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), GRect(4, 2, 136, 128), GTextOverflowModeFill, GTextAlignmentLeft).h + 12;
	}
	if (*product_get(cell_index->row)->surge) {
		return 48;
	}
	return 30;
}

static void menu_draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context) {
	graphics_context_set_text_color(ctx, GColorBlack);
	if (uber_get_error()) {
		graphics_draw_text(ctx, uber_get_error(), fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), GRect(4, 2, 136, 128), GTextOverflowModeFill, GTextAlignmentLeft, NULL);
	} else if (product_get_error()) {
		graphics_draw_text(ctx, product_get_error(), fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), GRect(4, 2, 136, 128), GTextOverflowModeFill, GTextAlignmentLeft, NULL);
	} else {
		Product *product = product_get(cell_index->row);
		graphics_draw_bitmap_in_rect(ctx, product->resource.image, product->resource.bounds);
		graphics_draw_text(ctx, product->name, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), GRect(24, 2, 116, 20), GTextOverflowModeFill, GTextAlignmentLeft, NULL);
		graphics_draw_text(ctx, product->estimate, fonts_get_system_font(FONT_KEY_GOTHIC_18), GRect(100, 2, 42, 22), GTextOverflowModeFill, GTextAlignmentRight, NULL);
		if (*product->surge) {
			graphics_draw_text(ctx, product->surge, fonts_get_system_font(FONT_KEY_GOTHIC_18), GRect(24, 22, 116, 20), GTextOverflowModeFill, GTextAlignmentLeft, NULL);
			graphics_draw_bitmap_in_rect(ctx, surge, GRect(5, 28, 12, 13));
		}
	}
}

static void menu_select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
	if (!product_count()) return;
	product_set_current(cell_index->row);
	locations_show();
}

static void menu_select_long_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
	uber_request_products();
}
