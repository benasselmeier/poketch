#include <pebble.h>

static Window *s_window;
static Layer *s_layer;
static GRect s_side_button_rect;
static int s_counter = 0;

static void set_counter(int value) {
  s_counter = value;
  layer_mark_dirty(s_layer);
}

static GRect inset_rect(GRect rect, int inset) {
  return GRect(rect.origin.x + inset,
               rect.origin.y + inset,
               rect.size.w - inset * 2,
               rect.size.h - inset * 2);
}

static void update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  // Background matches the Poketch's green field instead of a watchface shell.
  graphics_context_set_fill_color(ctx, GColorFromRGB(116, 181, 103));
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // Main counter screen.
  GRect screen_outer = GRect(8, 10, bounds.size.w - 26, 92);
  GRect screen_inner = inset_rect(screen_outer, 5);
  graphics_context_set_fill_color(ctx, GColorFromRGB(90, 145, 81));
  graphics_fill_rect(ctx, screen_outer, 8, GCornersAll);
  graphics_context_set_fill_color(ctx, GColorFromRGB(124, 187, 109));
  graphics_fill_rect(ctx, screen_inner, 6, GCornersAll);

  // Counter text.
  char counter_text[16];
  snprintf(counter_text, sizeof(counter_text), "%05d", s_counter);
  GRect counter_box = GRect(screen_inner.origin.x + 4, screen_inner.origin.y + 14, screen_inner.size.w - 8, 42);
  graphics_context_set_text_color(ctx, GColorFromRGB(24, 58, 31));
  graphics_draw_text(ctx, counter_text, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD), counter_box, GTextOverflowModeFill, GTextAlignmentCenter, NULL);

  // C button below the display.
  GRect c_outer = GRect(bounds.origin.x + 44, 112, 56, 46);
  GRect c_inner = inset_rect(c_outer, 4);
  graphics_context_set_fill_color(ctx, GColorFromRGB(74, 121, 67));
  graphics_fill_rect(ctx, c_outer, 0, GCornerNone);
  graphics_context_set_fill_color(ctx, GColorFromRGB(124, 187, 109));
  graphics_fill_rect(ctx, c_inner, 0, GCornerNone);
  graphics_context_set_text_color(ctx, GColorFromRGB(24, 58, 31));
  graphics_draw_text(ctx, "C", fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD), c_inner, GTextOverflowModeFill, GTextAlignmentCenter, NULL);

  // Right-side button bar: one bar with two buttons.
  s_side_button_rect = GRect(bounds.size.w - 15, 0, 15, bounds.size.h);
  GRect bar = s_side_button_rect;
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bar, 0, GCornerNone);

  GRect top_button = GRect(bar.origin.x + 1, 0, bar.size.w - 1, bounds.size.h / 2 - 1);
  GRect bottom_button = GRect(bar.origin.x + 1, bounds.size.h / 2 + 1, bar.size.w - 1, bounds.size.h - (bounds.size.h / 2) - 1);
  graphics_context_set_fill_color(ctx, GColorFromRGB(249, 74, 88));
  graphics_fill_rect(ctx, top_button, 4, GCornersRight);
  graphics_fill_rect(ctx, bottom_button, 4, GCornersRight);
  graphics_context_set_stroke_color(ctx, GColorFromRGB(120, 24, 31));
  graphics_draw_line(ctx, GPoint(bar.origin.x + 1, bounds.size.h / 2), GPoint(bar.origin.x + bar.size.w - 1, bounds.size.h / 2));
}

static void increment_counter(void) {
  if (s_counter >= 99999) {
    set_counter(0);
  } else {
    set_counter(s_counter + 1);
  }
}

static void decrement_counter(void) {
  if (s_counter > 0) {
    set_counter(s_counter - 1);
  }
}

static void reset_counter(void) {
  set_counter(0);
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  reset_counter();
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  increment_counter();
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  decrement_counter();
}

#if PBL_API_EXISTS(touch_service_subscribe)
// TouchService handler for SDK 4.9+: receives a TouchEvent and a context pointer.
// Increment the counter only when the touch position falls inside the side-button rect.
static void touch_handler(const TouchEvent *event, void *context) {
  if (!event) return;
  GPoint pt = GPoint(event->x, event->y);
  if (grect_contains_point(&s_side_button_rect, &pt)) {
    increment_counter();
  }
}
#endif

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
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

  // Force initial draw so side-button rect is defined before input arrives
  layer_mark_dirty(window_get_root_layer(s_window));
}

static void deinit(void) {
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
