#include <pebble.h>
#include "locations.h"
#include "libs/pebble-assist.h"
#include "uber.h"
#include "location.h"
#include "product.h"

static uint16_t menu_get_num_rows_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context);
static int16_t menu_get_header_height_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context);
static int16_t menu_get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context);
static void menu_draw_header_callback(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *callback_context);
static void menu_draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context);
static void menu_select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context);
static void menu_select_long_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context);

static Window *window = NULL;
static MenuLayer *menu_layer = NULL;

void locations_init(void) {
	window = window_create();

	menu_layer = menu_layer_create_fullscreen(window);
	menu_layer_set_callbacks(menu_layer, NULL, (MenuLayerCallbacks) {
		.get_num_rows = menu_get_num_rows_callback,
		.get_header_height = menu_get_header_height_callback,
		.get_cell_height = menu_get_cell_height_callback,
		.draw_header = menu_draw_header_callback,
		.draw_row = menu_draw_row_callback,
		.select_click = menu_select_callback,
		.select_long_click = menu_select_long_callback,
	});
	menu_layer_set_click_config_onto_window(menu_layer, window);
	menu_layer_add_to_window(menu_layer, window);
}

void locations_show(void) {
	location_count() ? uber_request_price() : uber_request_locations();
	for (int i = 0; i < location_count(); i++) {
		memset(location_get(i)->estimate, 0x0, sizeof(location_get(i)->estimate));
		memset(location_get(i)->duration, 0x0, sizeof(location_get(i)->duration));
		memset(location_get(i)->distance, 0x0, sizeof(location_get(i)->distance));
	}
	window_stack_push(window, true);
	menu_layer_reload_data_and_mark_dirty(menu_layer);
}

void locations_deinit(void) {
	menu_layer_destroy_safe(menu_layer);
	window_destroy_safe(window);
}

void locations_reload_data_and_mark_dirty(void) {
	menu_layer_reload_data_and_mark_dirty(menu_layer);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - //

static uint16_t menu_get_num_rows_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context) {
	return location_count() ? location_count() : 1;
}

static int16_t menu_get_header_height_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context) {
	return 30;
}

static int16_t menu_get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
	if (location_get_error()) {
		return graphics_text_layout_get_content_size(location_get_error(), fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), GRect(4, 2, 136, 128), GTextOverflowModeFill, GTextAlignmentLeft).h + 12;
	}
	if (*location_get(cell_index->row)->estimate || *location_get(cell_index->row)->distance) {
		return 52;
	}
	return 36;
}

static void menu_draw_header_callback(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *callback_context) {
	Product *product = product_get_current();
	graphics_context_set_text_color(ctx, GColorBlack);
	graphics_draw_bitmap_in_rect(ctx, product->resource.image, product->resource.bounds);
	graphics_draw_text(ctx, product->name, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), GRect(24, 2, 116, 20), GTextOverflowModeFill, GTextAlignmentLeft, NULL);
	graphics_draw_text(ctx, product->estimate, fonts_get_system_font(FONT_KEY_GOTHIC_18), GRect(100, 2, 42, 20), GTextOverflowModeFill, GTextAlignmentRight, NULL);
	graphics_draw_line(ctx, GPoint(0, 28), GPoint(144, 28));
}

static void menu_draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context) {
	graphics_context_set_text_color(ctx, GColorBlack);
	if (location_get_error()) {
		graphics_draw_text(ctx, location_get_error(), fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), GRect(4, 2, 136, 128), GTextOverflowModeFill, GTextAlignmentLeft, NULL);
	} else {
		Location *location = location_get(cell_index->row);
		graphics_draw_text(ctx, location->name, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GRect(4, 0, 136, 26), GTextOverflowModeFill, GTextAlignmentLeft, NULL);
		graphics_draw_text(ctx, location->estimate, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), GRect(4, 26, 136, 22), GTextOverflowModeFill, GTextAlignmentLeft, NULL);
		graphics_draw_text(ctx, location->duration, fonts_get_system_font(FONT_KEY_GOTHIC_18), GRect(4, 5, 136, 22), GTextOverflowModeFill, GTextAlignmentRight, NULL);
		graphics_draw_text(ctx, location->distance, fonts_get_system_font(FONT_KEY_GOTHIC_18), GRect(4, 26, 136, 22), GTextOverflowModeFill, GTextAlignmentRight, NULL);
	}
}

static void menu_select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
	if (!location_count()) return;
	location_set_current(cell_index->row);
}

static void menu_select_long_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
	uber_request_locations();
}
