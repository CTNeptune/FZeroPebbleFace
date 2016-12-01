#include <pebble.h>

static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_time_outline;
static Layer *s_battery_layer;
static int s_battery_level;
static BitmapLayer *s_bitmap_layer;
static GBitmap *s_bg_bitmap;

static BitmapLayer *s_charge_layer;
static GBitmap *s_charge_bitmap;

static BitmapLayer *s_nobt_layer;
static GBitmap *s_bg_nobt;



static void battery_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  // Find the width of the bar
  int width = (int)(float)(((float)s_battery_level / 100.0F) * 64.0F);

  // Draw the background
  graphics_context_set_fill_color(ctx, GColorClear);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // Draw the bar
  graphics_context_set_fill_color(ctx, GColorSunsetOrange);
  graphics_fill_rect(ctx, GRect(0, 0, width, bounds.size.h), 0, GCornerNone);
}
void battery_callback(BatteryChargeState state) {
  // Record the new battery level
  s_battery_level = state.charge_percent;
  if (state.is_charging) {
    layer_set_hidden(bitmap_layer_get_layer(s_charge_layer), false);
  } else{
    layer_set_hidden(bitmap_layer_get_layer(s_charge_layer), true);
  }
  // Update meter
  layer_mark_dirty(s_battery_layer);
}
static void bluetooth_callback(bool connected) {
  if(!connected) {
    // Issue a vibrating alert
    vibes_double_pulse();
    layer_set_hidden(bitmap_layer_get_layer(s_bitmap_layer), true);
  }else{
    layer_set_hidden(bitmap_layer_get_layer(s_bitmap_layer), false);
  }
}
static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Create a long-lived buffer
  static char buffer[] = "XXX XX 00:00 XX";
  // Write the current hour
  strftime(buffer, sizeof(buffer), "%b %d %l:%M %p", tick_time);
  
  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, buffer);
  text_layer_set_text(s_time_outline, buffer);
}
static void main_window_load(Window *window) {
  // Create TextLayer
  s_time_layer = text_layer_create(GRect(1, 151, 98, 14));
  s_time_outline = text_layer_create(GRect(2, 152, 98, 14));
  
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorYellow);
  
  text_layer_set_background_color(s_time_outline, GColorClear);
  text_layer_set_text_color(s_time_outline, GColorBlack);
  
  // Improve the layout to be more like a watchface
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentRight);
  
  text_layer_set_font(s_time_outline, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text_alignment(s_time_outline, GTextAlignmentRight);
  
  s_bg_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BG);
  s_bg_nobt = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_NOBT);
  s_charge_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SHIP);
  
  s_nobt_layer = bitmap_layer_create(GRect(0,0,144,168));
  bitmap_layer_set_bitmap(s_nobt_layer,s_bg_nobt);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_nobt_layer));
  
  s_bitmap_layer = bitmap_layer_create(GRect(110,158,24,7));
  bitmap_layer_set_bitmap(s_bitmap_layer,s_bg_bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_bitmap_layer));
  
  s_charge_layer = bitmap_layer_create(GRect(0,0,144,168));
  bitmap_layer_set_bitmap(s_charge_layer,s_charge_bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_charge_layer));
  bitmap_layer_set_compositing_mode(s_charge_layer, GCompOpSet);
  
  // Create battery meter Layer
  s_battery_layer = layer_create(GRect(34, 146, 64, 6));
  layer_set_update_proc(s_battery_layer, battery_update_proc);
  // Add to Window
  layer_add_child(window_get_root_layer(window), s_battery_layer);
  layer_mark_dirty(s_battery_layer);
  // Add text layer as a child layer to the Window's root layer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_outline));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));
  
}
static void main_window_unload(Window *window) {
  // Destroy TextLayer
  text_layer_destroy(s_time_layer);
}
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}
static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  
  // Make sure the time is displayed from the start
  update_time();
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  // Register for battery level updates
  battery_state_service_subscribe(battery_callback);
  // Ensure battery level is displayed from the start
  battery_callback(battery_state_service_peek());
  // Register for Bluetooth connection updates
  bluetooth_connection_service_subscribe(bluetooth_callback);
  // Show the correct state of the BT connection from the start
  bluetooth_callback(bluetooth_connection_service_peek());
  window_stack_push(s_main_window, true);
}
static void deinit() {
  // Destroy Window
  window_destroy(s_main_window);
  layer_destroy(s_battery_layer);
}
int main(void) {
  init();
  app_event_loop();
  deinit();
}
