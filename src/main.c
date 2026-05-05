#include <pebble.h>

static Window *s_window;
static Layer *s_layer;
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

static int min_int(int a, int b) {
  return a < b ? a : b;
}

static void update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  graphics_context_set_fill_color(ctx, GColorFromRGB(116, 181, 103));
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // Adaptive sizing for the original Pebble through Time 2.
  int side_pad = bounds.size.w / 12;
  if (side_pad < 6) side_pad = 6;
  int top_pad = bounds.size.h / 16;
  if (top_pad < 6) top_pad = 6;

  int screen_h = bounds.size.h / 2;
  if (screen_h < 56) screen_h = 56;
  if (screen_h > 92) screen_h = 92;

  GRect screen_outer = GRect(side_pad, top_pad, bounds.size.w - side_pad * 2, screen_h);
  int screen_inset = min_int(5, screen_outer.size.w / 16);
  if (screen_inset < 3) screen_inset = 3;
  GRect screen_inner = inset_rect(screen_outer, screen_inset);

  graphics_context_set_fill_color(ctx, GColorFromRGB(90, 145, 81));
  graphics_fill_rect(ctx, screen_outer, 8, GCornersAll);
  graphics_context_set_fill_color(ctx, GColorFromRGB(124, 187, 109));
  graphics_fill_rect(ctx, screen_inner, 6, GCornersAll);

  char counter_text[16];
  snprintf(counter_text, sizeof(counter_text), "%05d", s_counter);
  GRect counter_box = GRect(screen_inner.origin.x + 2,
                            screen_inner.origin.y + screen_inner.size.h / 6,
                            screen_inner.size.w - 4,
                            screen_inner.size.h * 2 / 3);
  graphics_context_set_text_color(ctx, GColorFromRGB(24, 58, 31));
  graphics_draw_text(ctx, counter_text,
                     fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD),
                     counter_box,
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentCenter,
                     NULL);

  // C button centered beneath the display.
  int c_w = bounds.size.w / 2;
  if (c_w < 48) c_w = 48;
  if (c_w > 72) c_w = 72;
  int c_h = bounds.size.h / 4;
  if (c_h < 34) c_h = 34;
  if (c_h > 52) c_h = 52;
  int c_y = screen_outer.origin.y + screen_outer.size.h + (bounds.size.h - (screen_outer.origin.y + screen_outer.size.h) - c_h) / 2;
  if (c_y + c_h > bounds.size.h - 4) {
    c_y = bounds.size.h - c_h - 4;
  }
  GRect c_outer = GRect((bounds.size.w - c_w) / 2, c_y, c_w, c_h);
  GRect c_inner = inset_rect(c_outer, 4);
  graphics_context_set_fill_color(ctx, GColorFromRGB(74, 121, 67));
  graphics_fill_rect(ctx, c_outer, 0, GCornerNone);
  graphics_context_set_fill_color(ctx, GColorFromRGB(124, 187, 109));
  graphics_fill_rect(ctx, c_inner, 0, GCornerNone);
  graphics_context_set_text_color(ctx, GColorFromRGB(24, 58, 31));
  graphics_draw_text(ctx, "C",
                     fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD),
                     c_inner,
                     GTextOverflowModeFill,
                     GTextAlignmentCenter,
                     NULL);
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
}

static void window_unload(Window *window) {
  layer_destroy(s_layer);
}

static void init(void) {
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_set_click_config_provider(s_window, click_config_provider);
  window_stack_push(s_window, true);
}

static void deinit(void) {
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
