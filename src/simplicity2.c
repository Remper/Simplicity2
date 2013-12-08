#include <pebble.h>

static Window *window;

//Constant for days
static char days[] = "MOTUWETHFRSASU";

//Layers
static TextLayer *text_date_layer;
static TextLayer *text_time_layer;
static TextLayer *text_day_layer;
static Layer *battery_layer;
static Layer *line_layer;

//Struct for application state management
typedef struct State {
	int battery_count;
	int wday;
	int month;
	int day;
} State;
static State state = {0, 0, 0, 0};

static void battery_layer_update_callback(Layer *me, GContext* ctx) {
	graphics_context_set_stroke_color(ctx, GColorWhite);
	graphics_context_set_fill_color(ctx, GColorWhite);

	graphics_draw_rect(ctx, GRect(8, 1, 28, 13));
	graphics_draw_rect(ctx, GRect(7, 0, 30, 15));
	graphics_fill_rect(ctx, GRect(37, 3, 3, 9), 0, GCornerNone);

	if (state.battery_count == -1) {
		graphics_draw_line(ctx, GPoint(15, 8), GPoint(22, 6));
		graphics_draw_line(ctx, GPoint(22, 6), GPoint(22, 8));
		graphics_draw_line(ctx, GPoint(22, 8), GPoint(29, 6));
		return;
	}

	for (int i = 0; i < state.battery_count; i++) {
		graphics_fill_rect(ctx, GRect(10+5*i, 3, 4, 9), 0, GCornerNone);
	}
}

static void line_layer_update_callback(Layer *me, GContext* ctx) {
	graphics_context_set_stroke_color(ctx, GColorWhite);
	graphics_context_set_fill_color(ctx, GColorWhite);

	graphics_draw_line(ctx, GPoint(8, 97), GPoint(131, 97));
	graphics_draw_line(ctx, GPoint(8, 98), GPoint(131, 98));
}

static void deinit() {
	window_destroy(window);
}

static void init() {
	window = window_create();
	window_stack_push(window, true /* Animated */);
	window_set_background_color(window, GColorBlack);
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);

	//Layer for current date "December 8"
	text_date_layer = text_layer_create(GRect(8, 68, bounds.size.w-8, bounds.size.h-68));
	text_layer_set_text_color(text_date_layer, GColorWhite);
	text_layer_set_background_color(text_date_layer, GColorClear);
	text_layer_set_font(text_date_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_CONDENSED_21)));
	layer_add_child(window_layer, text_layer_get_layer(text_date_layer));

	//Layer for current time "17:35"
	text_time_layer = text_layer_create(GRect(7, 92, bounds.size.w-7, bounds.size.h-92));
	text_layer_set_text_color(text_time_layer, GColorWhite);
	text_layer_set_background_color(text_time_layer, GColorClear);
	text_layer_set_font(text_time_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_BOLD_SUBSET_49)));
	layer_add_child(window_layer, text_layer_get_layer(text_time_layer));

	//Layer for current day of the week "SU"
	text_day_layer = text_layer_create(GRect(7, 145, bounds.size.w-7, bounds.size.h-92));
	text_layer_set_text_color(text_day_layer, GColorWhite);
	text_layer_set_background_color(text_day_layer, GColorClear);
	text_layer_set_font(text_day_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_CONDENSED_18)));
	layer_add_child(window_layer, text_layer_get_layer(text_day_layer));
	text_layer_set_text(text_day_layer, "YO");

	//Little line between date and time
	line_layer = layer_create(GRect(0, 0, bounds.size.w, bounds.size.h));
	layer_set_update_proc(line_layer, &line_layer_update_callback);
	layer_add_child(window_layer, line_layer);

	//Battery status icon
	battery_layer = layer_create(GRect(99, 4, 40, 40));
	layer_set_update_proc(battery_layer, &battery_layer_update_callback);
	layer_add_child(window_layer, battery_layer);
}


void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
	static char time_text[] = "00:00";
	static char date_text[] = "Xxxxxxxxx 00";
	static char day_text[] = "XX";

	char *time_format;
	int wday = tick_time->tm_wday - 1;
	if (wday == -1) {
		wday = 6;
	}

	if (tick_time->tm_mon != state.month || tick_time->tm_mday != state.day) {
		strftime(date_text, sizeof(date_text), "%B %e", tick_time);
		text_layer_set_text(text_date_layer, date_text);
		state.day = tick_time->tm_mday;
		state.month = tick_time->tm_mon;
	}

	if (wday != state.wday) {
		memmove(day_text, days + 2*wday, 2);
		layer_set_frame(text_layer_get_layer(text_day_layer), GRect(7 + 17*wday, 145, 30, 30));
		text_layer_set_text(text_day_layer, day_text);
		state.wday = wday;
	}

	if (clock_is_24h_style()) {
		time_format = "%R";
	} else {
		time_format = "%I:%M";
	}

	strftime(time_text, sizeof(time_text), time_format, tick_time);

	if (!clock_is_24h_style() && (time_text[0] == '0')) {
		memmove(time_text, &time_text[1], sizeof(time_text) - 1);
	}

	text_layer_set_text(text_time_layer, time_text);
}

static void handle_battery(BatteryChargeState charge_state) {
	layer_mark_dirty(battery_layer);
	if (charge_state.is_charging) {
		state.battery_count = -1;
		return;
	}

	state.battery_count = (charge_state.charge_percent / 10 + 1) / 2;
}

int main(void) {
	time_t curTime = time(NULL);

	init();
	tick_timer_service_subscribe(MINUTE_UNIT, &handle_minute_tick);
	battery_state_service_subscribe(&handle_battery);
	handle_battery(battery_state_service_peek());
	handle_minute_tick(localtime(&curTime), MINUTE_UNIT);
	app_event_loop();
	deinit();
}