#include <pebble.h>

static Window *window;

static TextLayer *text_date_layer;
static TextLayer *text_time_layer;
static TextLayer *battery_layer;

static Layer *line_layer;


void line_layer_update_callback(Layer *me, GContext* ctx) {

  graphics_context_set_stroke_color(ctx, GColorWhite);

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

  battery_layer = text_layer_create(GRect(8, 15, bounds.size.w-8, 168-10));
  text_layer_set_text_color(battery_layer, GColorWhite);
  text_layer_set_background_color(battery_layer, GColorClear);
  text_layer_set_font(battery_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_CONDENSED_18)));
  layer_add_child(window_layer, text_layer_get_layer(battery_layer));


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


  // TODO: Update display here to avoid blank display on launch?
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
  static char battery_text[] = "100% charged";

  if (charge_state.is_charging) {
	snprintf(battery_text, sizeof(battery_text), "Charging %d%%", charge_state.charge_percent);
  } else {
	snprintf(battery_text, sizeof(battery_text), "%d%% charged", charge_state.charge_percent);
  }
  
  text_layer_set_text(battery_layer, battery_text);
}

int main(void) {
  init();
  tick_timer_service_subscribe(MINUTE_UNIT, &handle_minute_tick);
  battery_state_service_subscribe(&handle_battery);
  handle_battery(battery_state_service_peek());
  app_event_loop();
  deinit();
}