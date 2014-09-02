#include <pebble.h>
#include "locations.h"
#include "../libs/pebble-assist.h"
#include "../uber.h"

static uint16_t menu_get_num_sections_callback(struct MenuLayer *menu_layer, void *callback_context);
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
		.get_num_sections = menu_get_num_sections_callback,
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
	window_stack_push(window, true);
	num_locations ? uber_request_price() : uber_request_locations();
	for (int i = 0; i < num_locations; i++) {
		strncpy(locations[i].estimate, "", sizeof(locations[i].estimate));
	}
}

void locations_deinit(void) {
	menu_layer_destroy_safe(menu_layer);
	window_destroy_safe(window);
}

void locations_reload_data_and_mark_dirty(void) {
	menu_layer_reload_data_and_mark_dirty(menu_layer);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - //

static uint16_t menu_get_num_sections_callback(struct MenuLayer *menu_layer, void *callback_context) {
	return 1;
}

static uint16_t menu_get_num_rows_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context) {
	return num_locations ? num_locations : 1;
}

static int16_t menu_get_header_height_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context) {
	return 28;
}

static int16_t menu_get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
	return 40;
}

static void menu_draw_header_callback(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *callback_context) {
	graphics_context_set_text_color(ctx, GColorBlack);
	graphics_draw_bitmap_in_rect(ctx, product()->image, product()->image_bounds);
	graphics_draw_text(ctx, product()->name, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), GRect(24, 2, 116, 20), GTextOverflowModeFill, GTextAlignmentLeft, NULL);
	graphics_draw_text(ctx, product()->estimate, fonts_get_system_font(FONT_KEY_GOTHIC_18), GRect(100, 2, 42, 20), GTextOverflowModeFill, GTextAlignmentRight, NULL);
}

static void menu_draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context) {
	graphics_context_set_text_color(ctx, GColorBlack);
	if (num_locations == 0) {
		graphics_draw_text(ctx, "Loading...", fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GRect(4, 2, 136, 26), GTextOverflowModeFill, GTextAlignmentLeft, NULL);
	} else {
		graphics_draw_text(ctx, locations[cell_index->row].name, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GRect(4, 2, 136, 26), GTextOverflowModeFill, GTextAlignmentLeft, NULL);
		graphics_draw_text(ctx, locations[cell_index->row].estimate, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), GRect(4, 7, 138, 22), GTextOverflowModeFill, GTextAlignmentRight, NULL);
	}
}

static void menu_select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
}

static void menu_select_long_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
	uber_request_locations();
}
