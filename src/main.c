#include <pebble.h>

#define SHOW_SIDE_APP_BUTTONS 0

static Window *s_window;
static Layer *s_layer;
static AppTimer *s_coin_timer;
static AppTimer *s_counter_hold_timer;

static GRect s_status_bar_rect;
static GRect s_viewport_rect;
static GRect s_softkey_dock_rect;
static GRect s_softkey_rects[3];
static GRect s_counter_plus_button_rect;

#if defined(PBL_PLATFORM_EMERY) && SHOW_SIDE_APP_BUTTONS
static GRect s_prev_app_button_rect;
static GRect s_next_app_button_rect;
#endif

#if defined(PBL_TOUCH)
static uint32_t s_last_touch_action_ms = 0;
#endif

static int s_counter = 0;
static bool s_coin_heads = true;
static bool s_coin_flipping = false;

typedef struct {
  GColor bg;
  GColor light;
  GColor mid;
  GColor dark;
} PoketchPalette;

typedef enum {
  APP_COUNTER = 0,
  APP_COIN = 1,
  APP_COUNT = 2,
} PoketchApp;

typedef enum {
  SOFTKEY_LEFT = 0,
  SOFTKEY_MIDDLE = 1,
  SOFTKEY_RIGHT = 2,
} SoftKey;

static PoketchApp s_active_app = APP_COUNTER;

static PoketchPalette current_palette(void) {
  return (PoketchPalette){
    GColorFromRGB(116, 181, 103),
    GColorFromRGB(124, 187, 109),
    GColorFromRGB(90, 145, 81),
    GColorFromRGB(24, 58, 31),
  };
}

static void redraw(void) {
  if (s_layer) layer_mark_dirty(s_layer);
}

static GRect inset_rect(GRect r, int i) {
  return GRect(r.origin.x + i, r.origin.y + i,
               r.size.w - i * 2, r.size.h - i * 2);
}

static const char *app_name(void) {
  return s_active_app == APP_COUNTER
    ? "Poketch - Counter"
    : "Poketch - Coin Flip";
}

static const char *softkey_label(SoftKey key) {
  if (s_active_app == APP_COUNTER) {
    if (key == SOFTKEY_LEFT) return "Reset";
    return "";
  }

  if (key == SOFTKEY_MIDDLE) return s_coin_flipping ? "..." : "Flip";
  return "";
}

static void finish_coin_flip(void *context) {
  s_coin_flipping = false;
  s_coin_heads = rand() % 2;
  s_coin_timer = NULL;
  vibes_short_pulse();
  redraw();
}

static void flip_coin(void) {
  if (s_coin_flipping) return;

  s_coin_flipping = true;
  redraw();

  s_coin_timer = app_timer_register(700, finish_coin_flip, NULL);
}

static void next_app(void) {
  s_active_app = (s_active_app + 1) % APP_COUNT;
  redraw();
}

static void prev_app(void) {
  int next = (int)s_active_app - 1;
  if (next < 0) next = APP_COUNT - 1;

  s_active_app = next;
  redraw();
}

static void run_softkey(SoftKey key) {
  if (s_active_app == APP_COUNTER) {
    if (key == SOFTKEY_LEFT) {
      s_counter = 0;
      redraw();
    }
    return;
  }

  if (s_active_app == APP_COIN && key == SOFTKEY_MIDDLE) {
    flip_coin();
  }
}

static void run_primary_action(void) {
  if (s_active_app == APP_COUNTER) {
    s_counter++;
    redraw();
  } else {
    flip_coin();
  }
}

static void counter_hold_timeout(void *context) {
  s_counter_hold_timer = NULL;
  if (s_active_app != APP_COUNTER) return;

  s_counter = 0;
  vibes_short_pulse();
  redraw();
}

#if defined(PBL_TOUCH)
static uint32_t now_millis(void) {
  time_t sec = 0;
  uint16_t ms = 0;
  time_ms(&sec, &ms);
  return (uint32_t)sec * 1000 + ms;
}
#endif

static void calculate_layout(GRect bounds) {
  int margin = 6;
  int status_h = 22;
  int dock_h = 34;

  s_status_bar_rect = GRect(margin, margin,
                            bounds.size.w - margin * 2,
                            status_h);

  s_softkey_dock_rect = GRect(margin,
                              bounds.size.h - margin - dock_h,
                              bounds.size.w - margin * 2,
                              dock_h);

  s_viewport_rect = GRect(margin,
                          s_status_bar_rect.origin.y + s_status_bar_rect.size.h,
                          bounds.size.w - margin * 2,
                          s_softkey_dock_rect.origin.y -
                          (s_status_bar_rect.origin.y + s_status_bar_rect.size.h));

  int gap = 4;
  int key_w = (s_softkey_dock_rect.size.w - gap * 2) / 3;

  for (int i = 0; i < 3; i++) {
    s_softkey_rects[i] = GRect(
      s_softkey_dock_rect.origin.x + i * (key_w + gap),
      s_softkey_dock_rect.origin.y,
      key_w,
      s_softkey_dock_rect.size.h
    );
  }

  int plus_size = 62;
  if (plus_size > s_viewport_rect.size.w - 12) plus_size = s_viewport_rect.size.w - 12;
  int plus_x = s_viewport_rect.origin.x + (s_viewport_rect.size.w - plus_size) / 2;
  int plus_y = s_viewport_rect.origin.y + s_viewport_rect.size.h - plus_size - 8;

  s_counter_plus_button_rect = GRect(plus_x, plus_y, plus_size, plus_size);
}

static void draw_status_bar(GContext *ctx) {
  PoketchPalette palette = current_palette();

  graphics_context_set_fill_color(ctx, palette.mid);
  graphics_fill_rect(ctx, s_status_bar_rect, 0, GCornerNone);

  graphics_context_set_text_color(ctx, palette.dark);

  graphics_draw_text(ctx,
                     app_name(),
                     fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     s_status_bar_rect,
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentCenter,
                     NULL);
}

static void draw_softkeys(GContext *ctx) {
  PoketchPalette palette = current_palette();

  graphics_context_set_fill_color(ctx, palette.mid);
  graphics_fill_rect(ctx, s_softkey_dock_rect, 0, GCornerNone);

  for (int i = 0; i < 3; i++) {
    const char *label = softkey_label((SoftKey)i);

    graphics_context_set_fill_color(ctx, palette.light);
    graphics_fill_rect(ctx, s_softkey_rects[i], 0, GCornerNone);

    graphics_context_set_stroke_color(ctx, palette.dark);
    graphics_draw_rect(ctx, s_softkey_rects[i]);

    graphics_context_set_text_color(ctx, palette.dark);

    graphics_draw_text(ctx,
                       label,
                       fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
                       inset_rect(s_softkey_rects[i], 2),
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentCenter,
                       NULL);
  }
}

static void draw_counter_plus_button(GContext *ctx, PoketchPalette palette) {
  graphics_context_set_fill_color(ctx, palette.mid);
  graphics_fill_rect(ctx, s_counter_plus_button_rect, 0, GCornerNone);

  graphics_context_set_stroke_color(ctx, palette.dark);
  graphics_draw_rect(ctx, s_counter_plus_button_rect);

  int cx = grect_center_point(&s_counter_plus_button_rect).x;
  int cy = grect_center_point(&s_counter_plus_button_rect).y;

  graphics_context_set_stroke_width(ctx, 5);
  graphics_draw_line(ctx, GPoint(cx - 14, cy), GPoint(cx + 14, cy));
  graphics_draw_line(ctx, GPoint(cx, cy - 14), GPoint(cx, cy + 14));
  graphics_context_set_stroke_width(ctx, 1);
}

static void draw_app_content(GContext *ctx) {
  PoketchPalette palette = current_palette();
  GRect content = inset_rect(s_viewport_rect, 8);

#if defined(PBL_PLATFORM_EMERY) && SHOW_SIDE_APP_BUTTONS
  content.size.w -= 34;
#endif

  graphics_context_set_text_color(ctx, palette.dark);

  if (s_active_app == APP_COUNTER) {
    char value_buf[16];
    snprintf(value_buf, sizeof(value_buf), "%d", s_counter);

    graphics_draw_text(ctx,
                       value_buf,
                       fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD),
                       GRect(content.origin.x,
                             content.origin.y + 10,
                             content.size.w,
                             50),
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentCenter,
                       NULL);

    draw_counter_plus_button(ctx, palette);

  } else {
    const char *coin_text = s_coin_flipping
      ? "FLIPPING..."
      : (s_coin_heads ? "HEADS" : "TAILS");

    graphics_draw_text(ctx,
                       coin_text,
                       fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD),
                       content,
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentCenter,
                       NULL);
  }
}

static void update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  calculate_layout(bounds);

  PoketchPalette palette = current_palette();

  graphics_context_set_fill_color(ctx, palette.mid);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  GRect shell = GRect(
    s_status_bar_rect.origin.x,
    s_status_bar_rect.origin.y,
    s_status_bar_rect.size.w,
    s_softkey_dock_rect.origin.y +
    s_softkey_dock_rect.size.h -
    s_status_bar_rect.origin.y
  );

  graphics_context_set_fill_color(ctx, palette.dark);
  graphics_fill_rect(ctx, shell, 0, GCornerNone);

  GRect shell_inner = inset_rect(shell, 4);

  graphics_context_set_fill_color(ctx, palette.bg);
  graphics_fill_rect(ctx, shell_inner, 0, GCornerNone);

  draw_status_bar(ctx);
  draw_app_content(ctx);
  draw_softkeys(ctx);
}

#if defined(PBL_TOUCH)

static bool is_touch_begin(const TouchEvent *event) {
#if defined(TOUCH_EVENT_TYPE_BEGIN)
  return event->type == TOUCH_EVENT_TYPE_BEGIN;
#elif defined(TOUCH_EVENT_TYPE_DOWN)
  return event->type == TOUCH_EVENT_TYPE_DOWN;
#else
  return true;
#endif
}

static bool is_touch_end_or_abort(const TouchEvent *event) {
#if defined(TOUCH_EVENT_TYPE_END) && defined(TOUCH_EVENT_TYPE_ABORT)
  return event->type == TOUCH_EVENT_TYPE_END ||
         event->type == TOUCH_EVENT_TYPE_ABORT;
#elif defined(TOUCH_EVENT_TYPE_UP) && defined(TOUCH_EVENT_TYPE_CANCEL)
  return event->type == TOUCH_EVENT_TYPE_UP ||
         event->type == TOUCH_EVENT_TYPE_CANCEL;
#elif defined(TOUCH_EVENT_TYPE_END)
  return event->type == TOUCH_EVENT_TYPE_END;
#elif defined(TOUCH_EVENT_TYPE_UP)
  return event->type == TOUCH_EVENT_TYPE_UP;
#else
  return false;
#endif
}

static void touch_handler(const TouchEvent *event, void *context) {
  if (!event) return;

  GPoint pt = GPoint(event->x, event->y);

  if (is_touch_begin(event)) {
    uint32_t now = now_millis();

    if (now - s_last_touch_action_ms < 250) return;

    for (int i = 0; i < 3; i++) {
      if (grect_contains_point(&s_softkey_rects[i], &pt)) {
        s_last_touch_action_ms = now;
        run_softkey((SoftKey)i);
        return;
      }
    }

    if (s_active_app == APP_COUNTER &&
        grect_contains_point(&s_counter_plus_button_rect, &pt)) {
      s_last_touch_action_ms = now;
      s_counter++;
      redraw();

      if (s_counter_hold_timer) {
        app_timer_cancel(s_counter_hold_timer);
      }
      s_counter_hold_timer = app_timer_register(3000, counter_hold_timeout, NULL);
      return;
    }
  }

  if (is_touch_end_or_abort(event)) {
    if (s_counter_hold_timer) {
      app_timer_cancel(s_counter_hold_timer);
      s_counter_hold_timer = NULL;
    }
  }
}
#endif

static void select_click_handler(ClickRecognizerRef r, void *c) {
  run_primary_action();
}

static void select_long_click_handler(ClickRecognizerRef r, void *c) {
  if (s_active_app == APP_COUNTER) {
    s_counter = 0;
    vibes_short_pulse();
    redraw();
    return;
  }

  run_softkey(SOFTKEY_LEFT);
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
  window_long_click_subscribe(BUTTON_ID_SELECT, 3000,
                              select_long_click_handler, NULL);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);

  s_layer = layer_create(layer_get_bounds(root));
  layer_set_update_proc(s_layer, update_proc);
  layer_add_child(root, s_layer);

#if defined(PBL_TOUCH)
  touch_service_subscribe(touch_handler, NULL);
#endif
}

static void window_unload(Window *window) {
#if defined(PBL_TOUCH)
  touch_service_unsubscribe();
#endif

  if (s_coin_timer) {
    app_timer_cancel(s_coin_timer);
    s_coin_timer = NULL;
  }

  if (s_counter_hold_timer) {
    app_timer_cancel(s_counter_hold_timer);
    s_counter_hold_timer = NULL;
  }

  layer_destroy(s_layer);
  s_layer = NULL;
}

static void init(void) {
  srand(time(NULL));

  s_window = window_create();

  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });

  window_set_click_config_provider(s_window,
                                   click_config_provider);

  window_stack_push(s_window, true);
}

static void deinit(void) {
  window_destroy(s_window);
  s_window = NULL;
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
