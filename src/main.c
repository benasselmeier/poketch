#include <pebble.h>

static Window *s_window;
static Layer *s_layer;
static int s_counter = 0;
static bool s_coin_heads = true;

typedef enum {
  APP_COUNTER = 0,
  APP_COIN = 1,
  APP_COUNT = 2,
} PoketchApp;

static PoketchApp s_active_app = APP_COUNTER;

static void redraw(void) {
  layer_mark_dirty(s_layer);
}

static void set_counter(int value) {
  s_counter = value;
  redraw();
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

static const char *platform_name(void) {
  #if defined(PBL_PLATFORM_APLITE)
    return "Aplite";
  #elif defined(PBL_PLATFORM_BASALT)
    return "Basalt";
  #elif defined(PBL_PLATFORM_CHALK)
    return "Chalk";
  #elif defined(PBL_PLATFORM_DIORITE)
    return "Diorite";
  #elif defined(PBL_PLATFORM_EMERY)
    return "Emery";
  #else
    return "Unknown";
  #endif
}

static void increment_counter(void) {
  if (s_counter >= 99999) {
    set_counter(0);
  } else {
    set_counter(s_counter + 1);
  }
}

static void flip_coin(void) {
  s_coin_heads = !s_coin_heads;
  redraw();
}

static void next_app(void) {
  s_active_app = (PoketchApp)((s_active_app + 1) % APP_COUNT);
  redraw();
}

static void prev_app(void) {
  int next = (int)s_active_app - 1;
  if (next < 0) next = APP_COUNT - 1;
  s_active_app = (PoketchApp)next;
  redraw();
}

static void update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  graphics_context_set_fill_color(ctx, GColorFromRGB(116, 181, 103));
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  int side_pad = bounds.size.w / 12;
  if (side_pad < 6) side_pad = 6;
  int top_pad = bounds.size.h / 16;
  if (top_pad < 6) top_pad = 6;

  int screen_h = (bounds.size.h * 9) / 16;
  if (screen_h < 56) screen_h = 56;
  if (screen_h > 120) screen_h = 120;

  GRect screen_outer = GRect(side_pad, top_pad, bounds.size.w - side_pad * 2, screen_h);
  int screen_inset = min_int(5, screen_outer.size.w / 16);
  if (screen_inset < 3) screen_inset = 3;
  GRect screen_inner = inset_rect(screen_outer, screen_inset);

  graphics_context_set_fill_color(ctx, GColorFromRGB(90, 145, 81));
  graphics_fill_rect(ctx, screen_outer, 8, GCornersAll);
  graphics_context_set_fill_color(ctx, GColorFromRGB(124, 187, 109));
  graphics_fill_rect(ctx, screen_inner, 6, GCornersAll);

  // App index indicator (top-left debug label): 1/2, 2/2, ...
  char app_index_text[8];
  snprintf(app_index_text, sizeof(app_index_text), "%d/%d", (int)s_active_app + 1, APP_COUNT);
  GRect app_index_box = GRect(screen_inner.origin.x + 3, screen_inner.origin.y + 1, 36, 14);
  graphics_context_set_text_color(ctx, GColorFromRGB(24, 58, 31));
  graphics_draw_text(ctx,
                     app_index_text,
                     fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     app_index_box,
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentLeft,
                     NULL);

  GRect platform_box = GRect(screen_inner.origin.x + 3, screen_inner.origin.y + 13, 56, 12);
  graphics_draw_text(ctx,
                     platform_name(),
                     fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     platform_box,
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentLeft,
                     NULL);

  if (s_active_app == APP_COUNTER) {
    char counter_text[16];
    if (s_counter < 100) {
      snprintf(counter_text, sizeof(counter_text), "%02d", s_counter);
    } else {
      snprintf(counter_text, sizeof(counter_text), "%d", s_counter);
    }
    GRect counter_box = GRect(screen_inner.origin.x + 2,
                              screen_inner.origin.y + screen_inner.size.h / 6,
                              screen_inner.size.w - 4,
                              screen_inner.size.h * 2 / 3);
    const char *counter_font_key = (bounds.size.w <= 144)
        ? FONT_KEY_BITHAM_30_BLACK
        : FONT_KEY_BITHAM_42_BOLD;
    graphics_draw_text(ctx, counter_text,
                       fonts_get_system_font(counter_font_key),
                       counter_box,
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentCenter,
                       NULL);
  } else {
    GRect coin_box = GRect(screen_inner.origin.x + 2,
                           screen_inner.origin.y + screen_inner.size.h / 4,
                           screen_inner.size.w - 4,
                           screen_inner.size.h / 2);
    const char *coin_font_key = (bounds.size.w >= 180)
        ? FONT_KEY_BITHAM_42_BOLD
        : FONT_KEY_GOTHIC_28_BOLD;
    graphics_draw_text(ctx,
                       s_coin_heads ? "HEADS" : "TAILS",
                       fonts_get_system_font(coin_font_key),
                       coin_box,
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentCenter,
                       NULL);
  }

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
  const char *c_font_key = (bounds.size.w <= 144)
      ? FONT_KEY_GOTHIC_24_BOLD
      : FONT_KEY_BITHAM_42_BOLD;
  graphics_draw_text(ctx,
                     s_active_app == APP_COUNTER ? "+" : "FLIP",
                     fonts_get_system_font(c_font_key),
                     c_inner,
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentCenter,
                     NULL);
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_active_app == APP_COUNTER) {
    increment_counter();
  } else {
    flip_coin();
  }
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  prev_app();
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  next_app();
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
