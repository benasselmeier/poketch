#include <pebble.h>

static Window *s_window;
static Layer *s_layer;
static AppTimer *s_kitchen_timer;

static int s_counter = 0;
static int s_steps = 6190;
static bool s_coin_heads = true;
static GRect s_action_button_rect;
static uint32_t s_last_touch_action_ms = 0;

static int s_timer_remaining_s = 5 * 60;
static bool s_timer_running = false;

typedef enum {
  APP_COUNTER = 0,
  APP_COIN = 1,
  APP_PEDOMETER = 2,
  APP_KITCHEN_TIMER = 3,
  APP_COUNT = 4,
} PoketchApp;

static PoketchApp s_active_app = APP_COUNTER;

static void redraw(void) { layer_mark_dirty(s_layer); }

static GRect inset_rect(GRect rect, int inset) {
  return GRect(rect.origin.x + inset, rect.origin.y + inset,
               rect.size.w - inset * 2, rect.size.h - inset * 2);
}

static int min_int(int a, int b) { return a < b ? a : b; }

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
  s_counter = (s_counter >= 99999) ? 0 : s_counter + 1;
  redraw();
}

static void flip_coin(void) {
  s_coin_heads = !s_coin_heads;
  redraw();
}

static void increment_steps(void) {
  s_steps += 10;
  if (s_steps > 99999) s_steps = 0;
  redraw();
}

static void kitchen_timer_tick(void *context) {
  if (!s_timer_running) return;
  if (s_timer_remaining_s > 0) s_timer_remaining_s--;
  if (s_timer_remaining_s > 0) {
    s_kitchen_timer = app_timer_register(1000, kitchen_timer_tick, NULL);
  } else {
    s_timer_running = false;
    vibes_short_pulse();
  }
  redraw();
}

static void toggle_kitchen_timer(void) {
  s_timer_running = !s_timer_running;
  if (s_timer_running) {
    s_kitchen_timer = app_timer_register(1000, kitchen_timer_tick, NULL);
  }
  redraw();
}

static void reset_kitchen_timer(void) {
  s_timer_running = false;
  s_timer_remaining_s = 5 * 60;
  redraw();
}
#endif

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

static void run_active_action(void) {
  switch (s_active_app) {
    case APP_COUNTER: increment_counter(); break;
    case APP_COIN: flip_coin(); break;
    case APP_PEDOMETER: increment_steps(); break;
    case APP_KITCHEN_TIMER: toggle_kitchen_timer(); break;
    default: break;
  }
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

  char app_index_text[8];
  snprintf(app_index_text, sizeof(app_index_text), "%d/%d", (int)s_active_app + 1, APP_COUNT);
  graphics_context_set_text_color(ctx, GColorFromRGB(24, 58, 31));
  graphics_draw_text(ctx, app_index_text, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     GRect(screen_inner.origin.x + 3, screen_inner.origin.y + 1, 42, 14),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  graphics_draw_text(ctx, platform_name(), fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     GRect(screen_inner.origin.x + 3, screen_inner.origin.y + 13, 56, 12),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

  if (s_active_app == APP_COUNTER || s_active_app == APP_PEDOMETER) {
    int value = (s_active_app == APP_COUNTER) ? s_counter : s_steps;
    char value_text[16];
    if (value < 100) snprintf(value_text, sizeof(value_text), "%02d", value);
    else snprintf(value_text, sizeof(value_text), "%d", value);

    graphics_draw_text(ctx, value_text,
                       fonts_get_system_font((bounds.size.w <= 144) ? FONT_KEY_BITHAM_30_BLACK : FONT_KEY_BITHAM_42_BOLD),
                       GRect(screen_inner.origin.x + 2, screen_inner.origin.y + screen_inner.size.h / 6,
                             screen_inner.size.w - 4, screen_inner.size.h * 2 / 3),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  } else if (s_active_app == APP_COIN) {
    graphics_draw_text(ctx, s_coin_heads ? "HEADS" : "TAILS",
                       fonts_get_system_font((bounds.size.w >= 180) ? FONT_KEY_GOTHIC_24_BOLD : FONT_KEY_GOTHIC_28_BOLD),
                       GRect(screen_inner.origin.x + 2, screen_inner.origin.y + screen_inner.size.h / 4,
                             screen_inner.size.w - 4, screen_inner.size.h / 2),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  } else {
    int min = s_timer_remaining_s / 60;
    int sec = s_timer_remaining_s % 60;
    char timer_text[8];
    snprintf(timer_text, sizeof(timer_text), "%02d:%02d", min, sec);
    graphics_draw_text(ctx, ":)", fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
                       GRect(screen_inner.origin.x, screen_inner.origin.y + 16, screen_inner.size.w, 20),
                       GTextOverflowModeFill, GTextAlignmentCenter, NULL);
    graphics_draw_text(ctx, timer_text,
                       fonts_get_system_font((bounds.size.w <= 144) ? FONT_KEY_BITHAM_30_BLACK : FONT_KEY_BITHAM_42_BOLD),
                       GRect(screen_inner.origin.x + 2, screen_inner.origin.y + screen_inner.size.h / 3,
                             screen_inner.size.w - 4, screen_inner.size.h / 2),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  }

  int c_w = bounds.size.w / 2;
  if (c_w < 48) c_w = 48;
  if (c_w > 72) c_w = 72;
  int c_h = bounds.size.h / 4;
  if (c_h < 34) c_h = 34;
  if (c_h > 52) c_h = 52;
  int c_y = screen_outer.origin.y + screen_outer.size.h + (bounds.size.h - (screen_outer.origin.y + screen_outer.size.h) - c_h) / 2;
  if (c_y + c_h > bounds.size.h - 4) c_y = bounds.size.h - c_h - 4;

  GRect c_outer = GRect((bounds.size.w - c_w) / 2, c_y, c_w, c_h);
  s_action_button_rect = c_outer;
  GRect c_inner = inset_rect(c_outer, 4);
  graphics_context_set_fill_color(ctx, GColorFromRGB(74, 121, 67));
  graphics_fill_rect(ctx, c_outer, 0, GCornerNone);
  graphics_context_set_fill_color(ctx, GColorFromRGB(124, 187, 109));
  graphics_fill_rect(ctx, c_inner, 0, GCornerNone);

  const char *label = "+";
  if (s_active_app == APP_COIN) label = "FLIP";
  if (s_active_app == APP_PEDOMETER) label = "+10";
  if (s_active_app == APP_KITCHEN_TIMER) label = s_timer_running ? "STOP" : "START";
  graphics_draw_text(ctx, label, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), c_inner,
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
}

#if PBL_API_EXISTS(touch_service_subscribe)
static uint32_t now_millis(void) {
  time_t sec = 0;
  uint16_t ms = 0;
  time_ms(&sec, &ms);
  return (uint32_t)sec * 1000 + ms;
}

static void touch_handler(const TouchEvent *event, void *context) {
  if (!event) return;
  GPoint pt = GPoint(event->x, event->y);
  if (!grect_contains_point(&s_action_button_rect, &pt)) return;
  uint32_t now = now_millis();
  if (now - s_last_touch_action_ms < 250) return;
  s_last_touch_action_ms = now;
  run_active_action();
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) { run_active_action(); }
static void select_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_active_app == APP_KITCHEN_TIMER) reset_kitchen_timer();
}
static void up_click_handler(ClickRecognizerRef recognizer, void *context) { prev_app(); }
static void down_click_handler(ClickRecognizerRef recognizer, void *context) { next_app(); }

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_long_click_subscribe(BUTTON_ID_SELECT, 500, select_long_click_handler, NULL);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  s_layer = layer_create(layer_get_bounds(window_layer));
  layer_set_update_proc(s_layer, update_proc);
  layer_add_child(window_layer, s_layer);
#if PBL_API_EXISTS(touch_service_subscribe)
  touch_service_subscribe(touch_handler, NULL);
#endif
}

static void window_unload(Window *window) {
#if PBL_API_EXISTS(touch_service_unsubscribe)
  touch_service_unsubscribe();
#endif
  if (s_kitchen_timer) app_timer_cancel(s_kitchen_timer);
  layer_destroy(s_layer);
}

static void init(void) {
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){ .load = window_load, .unload = window_unload });
  window_set_click_config_provider(s_window, click_config_provider);
  window_stack_push(s_window, true);
}

static void deinit(void) { window_destroy(s_window); }

int main(void) {
  init();
  app_event_loop();
  deinit();
}
