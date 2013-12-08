#include <pebble.h>

static Window *window;

static TextLayer *text_date_layer;
static TextLayer *text_time_layer;
static Layer *battery_layer;
int battery_count = 0;

static Layer *line_layer;

static void battery_layer_update_callback(Layer *me, GContext* ctx) {
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_context_set_fill_color(ctx, GColorWhite);
  
  graphics_draw_rect(ctx, GRect(8, 1, 28, 13));
  graphics_draw_rect(ctx, GRect(7, 0, 30, 15));
  graphics_fill_rect(ctx, GRect(37, 3, 3, 9), 0, GCornerNone);
  
  if (battery_count == -1) {
  	 graphics_draw_line(ctx, GPoint(12, 8), GPoint(22, 6));
  	 graphics_draw_line(ctx, GPoint(22, 6), GPoint(22, 8));
  	 graphics_draw_line(ctx, GPoint(22, 8), GPoint(32, 6));
  	 return;
  }
  
  int rect_points[5] = {10, 15, 20, 25, 30};
  
  for (int i = 0; i < battery_count; i++) {
     graphics_fill_rect(ctx, GRect(rect_points[i], 3, 4, 9), 0, GCornerNone);
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

  text_date_layer = text_layer_create(GRect(8, 68, bounds.size.w-8, 168-68));
  text_layer_set_text_color(text_date_layer, GColorWhite);
  text_layer_set_background_color(text_date_layer, GColorClear);
  text_layer_set_font(text_date_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_CONDENSED_21)));
  layer_add_child(window_layer, text_layer_get_layer(text_date_layer));


  text_time_layer = text_layer_create(GRect(7, 92, 144-7, 168-92));
  text_layer_set_text_color(text_time_layer, GColorWhite);
  text_layer_set_background_color(text_time_layer, GColorClear);
  text_layer_set_font(text_time_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_BOLD_SUBSET_49)));
  layer_add_child(window_layer, text_layer_get_layer(text_time_layer));

  line_layer = layer_create(GRect(0, 0, 144, 168));
  layer_set_update_proc(line_layer, &line_layer_update_callback);
  layer_add_child(window_layer, line_layer);

  battery_layer = layer_create(GRect(99, 4, 40, 40));
  layer_set_update_proc(battery_layer, &battery_layer_update_callback);
  layer_add_child(window_layer, battery_layer);
}


void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {

  // Need to be static because they're used by the system later.
  static char time_text[] = "00:00";
  static char date_text[] = "Xxxxxxxxx 00";

  char *time_format;


  // TODO: Only update the date when it's changed.
  strftime(date_text, sizeof(date_text), "%B %e", tick_time);
  text_layer_set_text(text_date_layer, date_text);


  if (clock_is_24h_style()) {
    time_format = "%R";
  } else {
    time_format = "%I:%M";
  }

  strftime(time_text, sizeof(time_text), time_format, tick_time);

  // Kludge to handle lack of non-padded hour format string
  // for twelve hour clock.
  if (!clock_is_24h_style() && (time_text[0] == '0')) {
    memmove(time_text, &time_text[1], sizeof(time_text) - 1);
  }

  text_layer_set_text(text_time_layer, time_text);
}

static void handle_battery(BatteryChargeState charge_state) {
  layer_mark_dirty(battery_layer);
  if (charge_state.is_charging) {
    battery_count = -1;
  	return;
  }
	
  battery_count = (charge_state.charge_percent / 10 + 1) / 2;
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