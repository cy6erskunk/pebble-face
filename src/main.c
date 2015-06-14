#include <pebble.h>
  
#define KEY_TEMPERATURE 0
#define KEY_CONDITIONS 1
  
static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_weather_layer;
static TextLayer *s_wday_layer;
static TextLayer *s_date_layer;
static GFont s_time_font;
static GFont s_weather_font;
static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;

static void update_time() {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  
  static char buffer[] = "00:00";
  static char wday_buffer[] = "Mon";
  static char date_buffer[] = "September 15";
  
  if (clock_is_24h_style() == true) {
    strftime(buffer, sizeof("00:00"), "%H:%M", tick_time);
  } else {
    strftime(buffer, sizeof("00:00"), "%I:%M", tick_time);
  }
  
  strftime(wday_buffer, sizeof("Mon"), "%a", tick_time);
  strftime(date_buffer, sizeof("September 15"), "%B %e", tick_time);
  
  text_layer_set_text(s_time_layer, buffer);
  text_layer_set_text(s_wday_layer, wday_buffer);
  text_layer_set_text(s_date_layer, date_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  if(tick_time->tm_min % 30 == 0) {
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
  
    // Add a key-value pair
    dict_write_uint8(iter, 0, 0);
  
    // Send the message!
    app_message_outbox_send();
  }
  update_time();
}

void init_text_layer(TextLayer *layer, GFont font, GColor color, GColor bg_color, GTextAlignment alignment) {
  text_layer_set_background_color(layer, bg_color);
  text_layer_set_text_color(layer, color);
  text_layer_set_font(layer, font);
  text_layer_set_text_alignment(layer, alignment);
}

static void main_window_load(Window *window) {
  s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BACKGROUND_TEST);
  s_background_layer = bitmap_layer_create(GRect(0, 0, 144, 168));
  bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_background_layer));
  
  s_time_layer = text_layer_create(GRect(5, 55, 139, 50));
  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PERFECT_DOS_42));
  
  init_text_layer(s_time_layer, s_time_font, GColorBlack, GColorClear, GTextAlignmentCenter);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));
  
  s_weather_layer = text_layer_create(GRect(5, 15, 139, 25));
  s_weather_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PERFECT_DOS_20));
  
  init_text_layer(s_weather_layer, s_weather_font, GColorWhite, GColorClear, GTextAlignmentCenter);
  text_layer_set_text(s_weather_layer, "Loading...");
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_weather_layer));
  
  s_wday_layer = text_layer_create(GRect(5, 115, 139, 25));
  init_text_layer(s_wday_layer, s_weather_font, GColorWhite, GColorClear, GTextAlignmentCenter);
  text_layer_set_text(s_wday_layer, "...");
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_wday_layer));
  
  s_date_layer = text_layer_create(GRect(5, 143, 139, 25));
  init_text_layer(s_date_layer, s_weather_font, GColorWhite, GColorClear, GTextAlignmentCenter);
  text_layer_set_text(s_date_layer, "...");
  
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer));
  
  update_time();
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_weather_layer);
  text_layer_destroy(s_wday_layer);
  text_layer_destroy(s_date_layer);
  
  fonts_unload_custom_font(s_time_font);
  fonts_unload_custom_font(s_weather_font);
  
  gbitmap_destroy(s_background_bitmap);
  
  bitmap_layer_destroy(s_background_layer);
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  Tuple *t = dict_read_first(iterator);
  static char temperature_buffer[8];
  static char conditions_buffer[32];
  static char weather_layer_buffer[32];
  
  // For all items
  while(t != NULL) {
    switch(t->key) {
    case KEY_TEMPERATURE:
      snprintf(temperature_buffer, sizeof(temperature_buffer), "%dC", (int)t->value->int32);
      break;
    case KEY_CONDITIONS:
      snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", t->value->cstring);
      break;
    default:
      APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
      break;
    }

    t = dict_read_next(iterator);
  }
  
  snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s, %s", temperature_buffer, conditions_buffer);
  text_layer_set_text(s_weather_layer, weather_layer_buffer);
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void init() {
  s_main_window = window_create();
  
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
    
  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  app_message_register_inbox_received(inbox_received_callback);
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
  
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

}

static void deinit() {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}