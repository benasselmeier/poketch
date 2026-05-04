#include <pebble.h>

static Window *s_window;
static Layer *s_layer;
static GRect s_side_button_rect;
static int s_counter = 0;

static void update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  // Background
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // Pokétch outer body
  GRect body = GRect(10, 12, bounds.size.w - 20, bounds.size.h - 24);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, body, 8, GCornersAll);

  // Screen area
  GRect screen = GRect(body.origin.x + 8, body.origin.y + 10, body.size.w - 16, body.size.h - 58);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, screen, 6, GCornersAll);

  // Side button on the right edge (UI element)
  s_side_button_rect = GRect(bounds.size.w - 18, bounds.size.h / 2 - 20, 12, 40);
  graphics_context_set_fill_color(ctx, GColorDarkGray);
  graphics_fill_rect(ctx, s_side_button_rect, 4, GCornersAll);

  // Draw current time centered in the screen area
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  static char time_text[6];
  if (clock_is_24h_style()) {
    strftime(time_text, sizeof(time_text), "%H:%M", t);
  } else {
    strftime(time_text, sizeof(time_text), "%I:%M", t);
  }
  GFont time_font = fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD);
  graphics_context_set_text_color(ctx, GColorWhite);
  graphics_draw_text(ctx, time_text, time_font, screen, GTextOverflowModeFill, GTextAlignmentCenter, NULL);

  // Small counter bubble above the side button
  char buf[16];
  snprintf(buf, sizeof(buf), "%d", s_counter);
  GRect bubble = GRect(s_side_button_rect.origin.x - 18, s_side_button_rect.origin.y - 12, 36, 22);
  graphics_context_set_fill_color(ctx, GColorRed);
  graphics_fill_rect(ctx, bubble, 10, GCornersAll);
  graphics_context_set_text_color(ctx, GColorWhite);
  graphics_draw_text(ctx, buf, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), bubble, GTextOverflowModeFill, GTextAlignmentCenter, NULL);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(s_layer);
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  s_counter++;
  layer_mark_dirty(s_layer);
}

#if PBL_API_EXISTS(touch_service_subscribe)
// TouchService handler for SDK 4.9+: receives a TouchEvent and a context pointer.
// Increment the counter only when the touch position falls inside the side-button rect.
static void touch_handler(const TouchEvent *event, void *context) {
  if (!event) return;
  GPoint pt = GPoint(event->x, event->y);
  if (grect_contains_point(&s_side_button_rect, &pt)) {
    s_counter++;
    layer_mark_dirty(s_layer);
  }
}
#endif

static void click_config_provider(void *context) {
  // Fallback input for non-touch Pebbles: use Select button
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  s_layer = layer_create(bounds);
  layer_set_update_proc(s_layer, update_proc);
  layer_add_child(window_layer, s_layer);

  // Subscribe to touch service if available (Pebble Time 2 / emery)
  #if PBL_API_EXISTS(touch_service_subscribe)
    touch_service_subscribe(touch_handler, NULL);
  #endif
}

static void window_unload(Window *window) {
  layer_destroy(s_layer);

  #if PBL_API_EXISTS(touch_service_unsubscribe)
    touch_service_unsubscribe();
  #endif
}

static void init(void) {
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_set_click_config_provider(s_window, click_config_provider);
  window_stack_push(s_window, true);

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  // Force initial draw so side-button rect is defined before input arrives
  layer_mark_dirty(window_get_root_layer(s_window));
}

static void deinit(void) {
  tick_timer_service_unsubscribe();
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
