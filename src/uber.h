#pragma once

#include <pebble.h>

void uber_init(void);
void uber_deinit(void);
void uber_request_products();
void uber_request_locations();
void uber_request_price();
void uber_reload_data_and_mark_dirty();
char* uber_get_error();
