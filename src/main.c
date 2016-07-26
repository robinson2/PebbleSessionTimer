#include <pebble.h>

Window *window;
MenuLayer *menu_layer;

#define MAX_TIMESTAMPS 50

uint32_t num_items_storage_key = 0;
uint32_t timestamp_storage_key = 1;

int num_items;
time_t timestamp[MAX_TIMESTAMPS];


/* ######################################################## */


void create_timestamp() {
	if(num_items < MAX_TIMESTAMPS) {
		timestamp[num_items] = time(NULL);
		num_items++;
		APP_LOG(APP_LOG_LEVEL_DEBUG, "created timestamp #%d\n", num_items);
	} else {
		// maybe later
		APP_LOG(APP_LOG_LEVEL_DEBUG, "already enough timestamps");
	}
}


void reset_timestamps() {
	// reset num_items
	num_items = 0;
	APP_LOG(APP_LOG_LEVEL_DEBUG, "timestamps resetted");
	// list needs at least one item:
	create_timestamp();
}


void menu_draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *context) {
	// get itemnr (list items in reverse order)
	int itemnr = num_items - cell_index->row - 1;

	// generate the title
	char cell_title[32];
	struct tm* ticktime = localtime(&timestamp[itemnr]);
	if(clock_is_24h_style() == true) strftime(cell_title,32, "%e.%m. %H:%M:%S", ticktime);
	else strftime(cell_title,32, "%m/%e %I:%M:%S %p", ticktime);

	// generate the subtitle (elapsed time since previous timestamp)
	char cell_subtitle[32];
	int diff;
	int h;
	int m;
	int s;
	if(itemnr > 0){
		diff = timestamp[itemnr] - timestamp[itemnr - 1];
		s = diff % 60; m = (diff / 60) % 60; h = (diff - s - (m * 60)) / 3600;

		if(h > 0) snprintf(cell_subtitle,32,"%dh %dm %ds",h,m,s);	
		else{
			if(m > 0) snprintf(cell_subtitle,32,"%dm %ds",m,s);	
			else snprintf(cell_subtitle,32,"%ds",s);	
		}
	}

	// draw the row
	menu_cell_basic_draw(ctx, cell_layer, cell_title, cell_subtitle, NULL);
}


uint16_t menu_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *callback_context) { return num_items; }


static int16_t menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  // This is a define provided in pebble.h that you may use for the default height
  return MENU_CELL_BASIC_HEADER_HEIGHT;
}


static void menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
	char header_title[32];
	time_t now = time(NULL);
	int diff;
	int h;
	int m;
	int s;
	
	diff = now - timestamp[num_items - 1];
	s = diff % 60; m = (diff / 60) % 60; h = (diff - s - (m * 60)) / 3600;
	
	if(h > 0) snprintf(header_title,32,"  %d/%d  —  %dh %dm %ds",num_items,MAX_TIMESTAMPS,h,m,s);	
	else{
		if(m > 0) snprintf(header_title,32,"  %d/%d  —  %dm %ds",num_items,MAX_TIMESTAMPS,m,s);	
		else snprintf(header_title,32,"  %d/%d  —  %ds",num_items,MAX_TIMESTAMPS,s);		
	}
	
	menu_cell_basic_header_draw(ctx, cell_layer, header_title);
}


void menu_select_click_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
	create_timestamp();
	// reload menu
	menu_layer_reload_data(menu_layer);
	// vibrate
	vibes_short_pulse();
}


void menu_select_long_click_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
	reset_timestamps();
	// reload menu
	menu_layer_reload_data(menu_layer);
	// vibrate
	vibes_double_pulse();
}


static void tick_handler(struct tm *tick_time, TimeUnits changed) { menu_layer_reload_data(menu_layer); }


void window_load(Window *window) {
	// Create menu_layer
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);
	menu_layer = menu_layer_create(bounds);
	
	// Let it receive clicks
	menu_layer_set_click_config_onto_window(menu_layer, window);
	
	#if defined(PBL_COLOR)
		menu_layer_set_normal_colors(menu_layer, GColorBlack, GColorWhite);
		menu_layer_set_highlight_colors(menu_layer, GColorRed, GColorWhite);
	#endif
	
	// Give it its callbacks
	menu_layer_set_callbacks(menu_layer, NULL, (MenuLayerCallbacks) {
		.get_num_rows = menu_num_rows_callback,
		.draw_row = menu_draw_row_callback,
		.get_header_height = menu_get_header_height_callback,
		.draw_header = menu_draw_header_callback,
		.select_click = menu_select_click_callback,
		.select_long_click = menu_select_long_click_callback,
	});
	
	// Add to Window
	layer_add_child(window_layer, menu_layer_get_layer(menu_layer));
	
	tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
}


void window_unload(Window *window) {
	menu_layer_destroy(menu_layer);
	APP_LOG(APP_LOG_LEVEL_DEBUG, "menu_layer destroied");
}


void read_persist_values() {
	if(persist_exists(timestamp_storage_key)) persist_read_data(timestamp_storage_key, timestamp, sizeof(timestamp));
	if(persist_exists(num_items_storage_key)){
		num_items = persist_read_int(num_items_storage_key);
		APP_LOG(APP_LOG_LEVEL_DEBUG, "num_items after read num_items_storage_key: %d\n", num_items);
	} else{
		num_items = 0;
		APP_LOG(APP_LOG_LEVEL_DEBUG, "create initial timestamp on first start ...");
		create_timestamp();
	}
}


void write_persist_values() {
	persist_write_int(num_items_storage_key, num_items);
	persist_write_data(timestamp_storage_key, timestamp, sizeof(timestamp));
	APP_LOG(APP_LOG_LEVEL_DEBUG, "stored persists values");
}


void init() {
	// read values 
	read_persist_values();
	// create the window
	window = window_create();
	window_set_window_handlers(window, (WindowHandlers) {
		.load = window_load,
		.unload = window_unload,
	});
	// push to the stack, animated
	window_stack_push(window, true);
}


void deinit() {
	// save values to persistent storage
	write_persist_values();
	// destroy the window
	window_destroy(window);
	APP_LOG(APP_LOG_LEVEL_DEBUG, "window destroied");
}


int main(void) {
	// Initialize the app
	init();
	// Wait for app events
	app_event_loop();
	// Deinitialize the app
	deinit();
	// App finished without error
	return 0;
}