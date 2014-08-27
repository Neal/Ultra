#pragma once

void products_init(void);
void products_deinit(void);
void products_reload_data_and_mark_dirty(void);
uint32_t products_get_resource_id(int i);
GRect products_get_resource_image_rect(int i);
