#include <pebble.h>
#include "win-locations.h"
#include "libs/pebble-assist.h"
#include "uber.h"
#include "locations.h"
#include "products.h"

static uint16_t menu_get_num_rows_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context);
static int16_t menu_get_header_height_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context);
static int16_t menu_get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context);
static void menu_draw_header_callback(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *callback_context);
static void menu_draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context);
static void menu_select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context);
static void menu_select_long_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context);

static Window *window = NULL;
static MenuLayer *menu_layer = NULL;

void win_locations_init(void) {
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

void win_locations_show(void) {
	locations_count() ? uber_request_price() : uber_request_locations();
	for (int i = 0; i < locations_count(); i++) {
		memset(locations_get(i)->estimate, 0x0, sizeof(locations_get(i)->estimate));
		memset(locations_get(i)->duration, 0x0, sizeof(locations_get(i)->duration));
		memset(locations_get(i)->distance, 0x0, sizeof(locations_get(i)->distance));
	}
	window_stack_push(window, true);
	menu_layer_reload_data_and_mark_dirty(menu_layer);
}

void win_locations_deinit(void) {
	menu_layer_destroy_safe(menu_layer);
	window_destroy_safe(window);
}

void win_locations_reload_data_and_mark_dirty(void) {
	menu_layer_reload_data_and_mark_dirty(menu_layer);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - //

static uint16_t menu_get_num_rows_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context) {
	return locations_count() ? locations_count() : 1;
}

static int16_t menu_get_header_height_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context) {
	return 30;
}

static int16_t menu_get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
	if (locations_get_error()) {
		return graphics_text_layout_get_content_size(locations_get_error(), fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), GRect(4, 2, 136, 128), GTextOverflowModeFill, GTextAlignmentLeft).h + 12;
	}
	if (*locations_get(cell_index->row)->estimate || *locations_get(cell_index->row)->distance) {
		return 50;
	}
	return 32;
}

static void menu_draw_header_callback(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *callback_context) {
	Product *product = products_get_current();
	graphics_context_set_text_color(ctx, GColorBlack);
	graphics_draw_bitmap_in_rect(ctx, product->resource.image, product->resource.bounds);
	graphics_draw_text(ctx, product->name, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), GRect(24, 2, 116, 20), GTextOverflowModeFill, GTextAlignmentLeft, NULL);
	graphics_draw_text(ctx, product->estimate, fonts_get_system_font(FONT_KEY_GOTHIC_18), GRect(100, 2, 42, 20), GTextOverflowModeFill, GTextAlignmentRight, NULL);
	graphics_draw_line(ctx, GPoint(0, 28), GPoint(144, 28));
}

static void menu_draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context) {
	graphics_context_set_text_color(ctx, GColorBlack);
	if (locations_get_error()) {
		graphics_draw_text(ctx, locations_get_error(), fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), GRect(4, 2, 136, 128), GTextOverflowModeFill, GTextAlignmentLeft, NULL);
	} else {
		Location *location = locations_get(cell_index->row);
		graphics_draw_text(ctx, location->name, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GRect(4, -2, 136, 26), GTextOverflowModeFill, GTextAlignmentLeft, NULL);
		graphics_draw_text(ctx, location->estimate, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), GRect(4, 25, 136, 22), GTextOverflowModeFill, GTextAlignmentLeft, NULL);
		graphics_draw_text(ctx, location->duration, fonts_get_system_font(FONT_KEY_GOTHIC_18), GRect(4, 3, 136, 22), GTextOverflowModeFill, GTextAlignmentRight, NULL);
		graphics_draw_text(ctx, location->distance, fonts_get_system_font(FONT_KEY_GOTHIC_18), GRect(4, 25, 136, 22), GTextOverflowModeFill, GTextAlignmentRight, NULL);
	}
}

static void menu_select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
	if (!locations_count()) return;
	locations_set_current(cell_index->row);
}

static void menu_select_long_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
	uber_request_locations();
}
