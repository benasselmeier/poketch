#include <pebble.h>
#include "forces_recorder.h"
#include "ride_data.h"
#include "parks.h"

#define PALETTE_COUNT 8

static Window *s_window;
static Layer *s_layer;
static AppTimer *s_tick_timer;

static GRect s_status_bar_rect;
static GRect s_viewport_rect;
static GRect s_softkey_dock_rect;
static GRect s_softkey_rects[3];

#if defined(PBL_TOUCH)
static uint32_t s_last_touch_action_ms = 0;
#endif

// App state machine
typedef enum {
  STATE_PARK_SELECT = 0,
  STATE_COASTER_SELECT = 1,
  STATE_RECORDING = 2,
  STATE_RIDE_HISTORY = 3,
} AppState;

static AppState s_state = STATE_PARK_SELECT;
static int s_park_idx = 0;
static int s_coaster_idx = 0;
static int s_history_idx = 0;
static int s_scroll_offset = 0;  // For scrollable lists

static int s_active_palette = 0;

typedef struct {
  const char *name;
  GColor bg;
  GColor light;
  GColor mid;
  GColor dark;
} PoketchPalette;

static PoketchPalette palette_get(int index) {
  switch (index) {
    case 0: return (PoketchPalette){ "GREEN",     GColorFromRGB(116,181,103), GColorFromRGB(124,187,109), GColorFromRGB(90,145,81),  GColorFromRGB(24,58,31) };
    case 1: return (PoketchPalette){ "YELLOW",    GColorFromRGB(194,188,105), GColorFromRGB(214,209,129), GColorFromRGB(132,128,66), GColorFromRGB(50,48,26) };
    case 2: return (PoketchPalette){ "ORANGE",    GColorFromRGB(205,143,82),  GColorFromRGB(221,161,96),  GColorFromRGB(132,86,48),  GColorFromRGB(52,34,20) };
    case 3: return (PoketchPalette){ "RED",       GColorFromRGB(214,105,105), GColorFromRGB(232,126,126), GColorFromRGB(132,48,48),  GColorFromRGB(58,22,22) };
    case 4: return (PoketchPalette){ "PURPLE",    GColorFromRGB(164,105,181), GColorFromRGB(184,126,202), GColorFromRGB(92,54,112),  GColorFromRGB(38,24,52) };
    case 5: return (PoketchPalette){ "BLUE",      GColorFromRGB(128,128,235), GColorFromRGB(148,148,255), GColorFromRGB(62,78,178),  GColorFromRGB(18,28,58) };
    case 6: return (PoketchPalette){ "TURQUOISE", GColorFromRGB(88,184,190),  GColorFromRGB(112,210,214), GColorFromRGB(35,124,130), GColorFromRGB(18,48,50) };
    default: return (PoketchPalette){ "WHITE",    GColorFromRGB(170,170,170), GColorFromRGB(210,210,210), GColorFromRGB(100,100,100),GColorFromRGB(24,24,24) };
  }
}

static PoketchPalette current_palette(void) {
  return palette_get(s_active_palette);
}

static void redraw(void) {
  if (s_layer) layer_mark_dirty(s_layer);
}

static GRect inset_rect(GRect r, int i) {
  return GRect(r.origin.x + i, r.origin.y + i,
               r.size.w - i * 2, r.size.h - i * 2);
}

static const char *app_name(void) {
  return "Ride Forces Recorder";
}

static const char *softkey_label(int key) {
  if (s_state == STATE_PARK_SELECT) {
    if (key == 0) return ""; // Left
    if (key == 1) return "SELECT"; // Middle
    return ""; // Right
  } else if (s_state == STATE_COASTER_SELECT) {
    if (key == 0) return "BACK"; // Left
    if (key == 1) return "SELECT"; // Middle
    return ""; // Right
  } else if (s_state == STATE_RECORDING) {
    if (key == 0) return "STOP"; // Left
    if (key == 1) return forces_recorder_is_recording() ? "PAUSE" : "REC"; // Middle
    return ""; // Right
  } else if (s_state == STATE_RIDE_HISTORY) {
    if (key == 0) return "BACK"; // Left
    if (key == 1) return "SELECT"; // Middle
    return ""; // Right
  }
  return "";
}

// State handlers
static void enter_park_select(void) {
  s_state = STATE_PARK_SELECT;
  s_park_idx = 0;
  s_scroll_offset = 0;
  redraw();
}

static void enter_coaster_select(void) {
  s_state = STATE_COASTER_SELECT;
  s_coaster_idx = 0;
  s_scroll_offset = 0;
  redraw();
}

static void enter_recording(void) {
  s_state = STATE_RECORDING;
  s_scroll_offset = 0;
  forces_recorder_start();
  s_tick_timer = app_timer_register(500, NULL, NULL);
  redraw();
}

static void enter_history(void) {
  s_state = STATE_RIDE_HISTORY;
  s_history_idx = 0;
  s_scroll_offset = 0;
  redraw();
}

// Actions for each state
static void softkey_action(int key) {
  if (s_state == STATE_PARK_SELECT) {
    if (key == 1) { // SELECT
      enter_coaster_select();
    }
  } else if (s_state == STATE_COASTER_SELECT) {
    if (key == 0) { // BACK
      enter_park_select();
    } else if (key == 1) { // SELECT
      enter_recording();
    }
  } else if (s_state == STATE_RECORDING) {
    if (key == 0) { // STOP
      forces_recorder_stop();
      // Save ride
      Park *park = parks_get(s_park_idx);
      Coaster *coaster = parks_get_coaster(s_park_idx, s_coaster_idx);
      if (park && coaster) {
        RideRecord ride = {
          .park = {0},
          .coaster = {0},
          .duration_s = forces_recorder_get_elapsed_s(),
          .max_g = forces_recorder_get_max_g(),
          .min_g = forces_recorder_get_min_g(),
          .timestamp = time(NULL),
        };
        strcpy(ride.park, park->name);
        strcpy(ride.coaster, coaster->name);
        ride_data_add(&ride);
      }
      enter_history();
    } else if (key == 1) { // PAUSE/REC
      if (forces_recorder_is_recording()) {
        forces_recorder_stop();
      } else {
        forces_recorder_start();
      }
      redraw();
    }
  } else if (s_state == STATE_RIDE_HISTORY) {
    if (key == 0) { // BACK
      enter_park_select();
    }
  }
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
    const char *label = softkey_label(i);

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

static void draw_side_buttons(GContext *ctx) {
  // Removed for cleaner UI
}

static void draw_app_content(GContext *ctx) {
  PoketchPalette palette = current_palette();
  GRect content = inset_rect(s_viewport_rect, 6);
  int row_height = 28;  // Height of each list item
  int rows_visible = (content.size.h / row_height);
  
  graphics_context_set_text_color(ctx, palette.dark);

  if (s_state == STATE_PARK_SELECT) {
    int park_count = parks_get_count();
    
    // Draw list header
    graphics_draw_text(ctx, "Parks", fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                       GRect(content.origin.x, content.origin.y, content.size.w, 16),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
    
    // Calculate scroll position
    if (s_park_idx >= s_scroll_offset + rows_visible) {
      s_scroll_offset = s_park_idx - rows_visible + 1;
    }
    if (s_park_idx < s_scroll_offset) {
      s_scroll_offset = s_park_idx;
    }
    
    int y = content.origin.y + 18;
    for (int i = 0; i < rows_visible && i + s_scroll_offset < park_count; i++) {
      Park *park = parks_get(i + s_scroll_offset);
      if (park) {
        // Highlight selected item
        if (i + s_scroll_offset == s_park_idx) {
          graphics_context_set_fill_color(ctx, palette.light);
          graphics_fill_rect(ctx, GRect(content.origin.x - 2, y - 2, content.size.w + 4, row_height - 2), 0, GCornerNone);
          graphics_context_set_text_color(ctx, palette.dark);
        }
        
        graphics_draw_text(ctx, park->name, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                           GRect(content.origin.x + 2, y, content.size.w - 4, row_height),
                           GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
        y += row_height;
      }
    }

  } else if (s_state == STATE_COASTER_SELECT) {
    Park *park = parks_get(s_park_idx);
    if (park) {
      // Draw park name as header
      graphics_draw_text(ctx, park->name, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                         GRect(content.origin.x, content.origin.y, content.size.w, 16),
                         GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
      
      // Calculate scroll position
      if (s_coaster_idx >= s_scroll_offset + rows_visible) {
        s_scroll_offset = s_coaster_idx - rows_visible + 1;
      }
      if (s_coaster_idx < s_scroll_offset) {
        s_scroll_offset = s_coaster_idx;
      }
      
      int y = content.origin.y + 18;
      for (int i = 0; i < rows_visible && i + s_scroll_offset < park->coaster_count; i++) {
        Coaster *coaster = parks_get_coaster(s_park_idx, i + s_scroll_offset);
        if (coaster) {
          // Highlight selected item
          if (i + s_scroll_offset == s_coaster_idx) {
            graphics_context_set_fill_color(ctx, palette.light);
            graphics_fill_rect(ctx, GRect(content.origin.x - 2, y - 2, content.size.w + 4, row_height - 2), 0, GCornerNone);
            graphics_context_set_text_color(ctx, palette.dark);
          }
          
          const char *type = coaster->type == COASTER_TYPE_WOOD ? "🪵" : "⛓️";
          char item_text[64];
          snprintf(item_text, sizeof(item_text), "%s %s", type, coaster->name);
          graphics_draw_text(ctx, item_text, fonts_get_system_font(FONT_KEY_GOTHIC_18),
                             GRect(content.origin.x + 2, y, content.size.w - 4, row_height),
                             GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
          y += row_height;
        }
      }
    }

  } else if (s_state == STATE_RECORDING) {
    char buf[64];
    uint32_t elapsed = forces_recorder_get_elapsed_s();
    uint16_t max_g = forces_recorder_get_max_g();
    uint16_t min_g = forces_recorder_get_min_g();
    ForceReading reading = forces_recorder_get_current();

    // Coaster name
    Coaster *coaster = parks_get_coaster(s_park_idx, s_coaster_idx);
    if (coaster) {
      graphics_draw_text(ctx, coaster->name, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                         GRect(content.origin.x, content.origin.y + 2, content.size.w, 18),
                         GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
    }

    // Elapsed time
    snprintf(buf, sizeof(buf), "%02lu:%02lu", (unsigned long)(elapsed / 60), (unsigned long)(elapsed % 60));
    graphics_draw_text(ctx, buf, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD),
                       GRect(content.origin.x, content.origin.y + 22, content.size.w, 48),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);

    // Current G-force (green)
    graphics_context_set_text_color(ctx, GColorGreen);
    uint16_t g_whole = reading.magnitude / 1000;
    uint16_t g_frac = (reading.magnitude % 1000) / 10;
    snprintf(buf, sizeof(buf), "%u.%02u g", g_whole, g_frac);
    graphics_draw_text(ctx, buf, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
                       GRect(content.origin.x, content.origin.y + 70, content.size.w, 24),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);

    graphics_context_set_text_color(ctx, palette.dark);

    // Max/Min
    uint16_t max_whole = max_g / 1000;
    uint16_t max_frac = (max_g % 1000) / 10;
    snprintf(buf, sizeof(buf), "Max: %u.%02u", max_whole, max_frac);
    graphics_draw_text(ctx, buf, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(content.origin.x, content.origin.y + 98, content.size.w / 2, 16),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

    uint16_t min_whole = min_g / 1000;
    uint16_t min_frac = (min_g % 1000) / 10;
    snprintf(buf, sizeof(buf), "Min: %u.%02u", min_whole, min_frac);
    graphics_draw_text(ctx, buf, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(content.origin.x + content.size.w / 2, content.origin.y + 98, content.size.w / 2, 16),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);

  } else if (s_state == STATE_RIDE_HISTORY) {
    int ride_count = ride_data_count();
    if (ride_count == 0) {
      graphics_draw_text(ctx, "No rides recorded yet", fonts_get_system_font(FONT_KEY_GOTHIC_18),
                         content, GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
    } else {
      // Draw rides list header
      graphics_draw_text(ctx, "Rides", fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                         GRect(content.origin.x, content.origin.y, content.size.w, 16),
                         GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
      
      // Calculate scroll position
      if (s_history_idx >= s_scroll_offset + rows_visible) {
        s_scroll_offset = s_history_idx - rows_visible + 1;
      }
      if (s_history_idx < s_scroll_offset) {
        s_scroll_offset = s_history_idx;
      }
      
      int y = content.origin.y + 18;
      for (int i = 0; i < rows_visible && i + s_scroll_offset < ride_count; i++) {
        RideRecord *ride = ride_data_get(i + s_scroll_offset);
        if (ride) {
          // Highlight selected item
          if (i + s_scroll_offset == s_history_idx) {
            graphics_context_set_fill_color(ctx, palette.light);
            graphics_fill_rect(ctx, GRect(content.origin.x - 2, y - 2, content.size.w + 4, row_height - 2), 0, GCornerNone);
            graphics_context_set_text_color(ctx, palette.dark);
          }
          
          char item_text[96];
          uint16_t max_whole = ride->max_g / 1000;
          uint16_t max_frac = (ride->max_g % 1000) / 10;
          snprintf(item_text, sizeof(item_text), "%s (%u.%02ug)", ride->coaster, max_whole, max_frac);
          graphics_draw_text(ctx, item_text, fonts_get_system_font(FONT_KEY_GOTHIC_18),
                             GRect(content.origin.x + 2, y, content.size.w - 4, row_height),
                             GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
          y += row_height;
        }
      }
    }
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
  draw_side_buttons(ctx);
  draw_softkeys(ctx);
}

#if defined(PBL_TOUCH)
static void touch_handler(const TouchEvent *event, void *context) {
  if (!event) return;

  uint32_t now = now_millis();
  if (now - s_last_touch_action_ms < 250) return;
  s_last_touch_action_ms = now;

  GPoint pt = GPoint(event->x, event->y);

  for (int i = 0; i < 3; i++) {
    if (grect_contains_point(&s_softkey_rects[i], &pt)) {
      softkey_action(i);
      return;
    }
  }
}
#endif

static void select_click_handler(ClickRecognizerRef r, void *c) {
  softkey_action(1);  // Middle softkey
}

static void select_long_click_handler(ClickRecognizerRef r, void *c) {
  softkey_action(0);  // Left softkey
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Navigate up/prev in current state
  if (s_state == STATE_PARK_SELECT) {
    if (s_park_idx > 0) s_park_idx--;
  } else if (s_state == STATE_COASTER_SELECT) {
    Park *park = parks_get(s_park_idx);
    if (park && s_coaster_idx > 0) s_coaster_idx--;
  } else if (s_state == STATE_RIDE_HISTORY) {
    if (s_history_idx > 0) s_history_idx--;
  }
  redraw();
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Navigate down/next in current state
  if (s_state == STATE_PARK_SELECT) {
    if (s_park_idx < parks_get_count() - 1) s_park_idx++;
  } else if (s_state == STATE_COASTER_SELECT) {
    Park *park = parks_get(s_park_idx);
    if (park && s_coaster_idx < park->coaster_count - 1) s_coaster_idx++;
  } else if (s_state == STATE_RIDE_HISTORY) {
    int ride_count = ride_data_count();
    if (ride_count > 0 && s_history_idx < ride_count - 1) s_history_idx++;
  }
  redraw();
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_long_click_subscribe(BUTTON_ID_SELECT, 500,
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

  // Initialize app
  parks_init();
  ride_data_init();
  forces_recorder_init();
  enter_park_select();
}

static void window_unload(Window *window) {
#if defined(PBL_TOUCH)
  touch_service_unsubscribe();
#endif

  forces_recorder_deinit();
  ride_data_deinit();
  parks_deinit();

  if (s_tick_timer) {
    app_timer_cancel(s_tick_timer);
    s_tick_timer = NULL;
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

  window_set_click_config_provider(s_window, click_config_provider);

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
