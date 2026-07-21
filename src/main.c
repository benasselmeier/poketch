// Ride Forces Recorder - Main Application
// Uses standard Pebble MenuLayer for list navigation
#include <pebble.h>
#include "forces_recorder.h"
#include "ride_data.h"
#include "parks.h"

// App state
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

// UI Elements
static Window *s_window = NULL;
static MenuLayer *s_menu_layer = NULL;
static Window *s_recording_window = NULL;
static Layer *s_recording_layer = NULL;
static AppTimer *s_recording_timer = NULL;

// Forward declarations
static void show_recording_screen(void);
static void hide_recording_screen(void);
static void return_to_previous_state(void);
static void enter_state(AppState new_state);

// ===== Menu Layer Callbacks =====
static uint16_t menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
  return 1;
}

static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  if (s_state == STATE_PARK_SELECT) {
    return parks_get_count();
  } else if (s_state == STATE_COASTER_SELECT) {
    Park *park = parks_get(s_park_idx);
    return park ? park->coaster_count : 0;
  } else if (s_state == STATE_RIDE_HISTORY) {
    return ride_data_count();
  }
  return 0;
}

static int16_t menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static void menu_draw_header_callback(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
  const char *header = "Parks";
  if (s_state == STATE_COASTER_SELECT) {
    Park *park = parks_get(s_park_idx);
    if (park) {
      header = park->name;
    }
  } else if (s_state == STATE_RIDE_HISTORY) {
    header = "Ride History";
  }
  
  menu_cell_basic_draw(ctx, cell_layer, header, NULL, NULL);
}

static void menu_draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
  char buffer[128];
  
  if (s_state == STATE_PARK_SELECT) {
    Park *park = parks_get(cell_index->row);
    if (park) {
      snprintf(buffer, sizeof(buffer), "%d coasters", park->coaster_count);
      menu_cell_basic_draw(ctx, cell_layer, park->name, buffer, NULL);
    }
  } else if (s_state == STATE_COASTER_SELECT) {
    Coaster *coaster = parks_get_coaster(s_park_idx, cell_index->row);
    if (coaster) {
      const char *type = coaster->type == COASTER_TYPE_WOOD ? "Wood" : "Steel";
      menu_cell_basic_draw(ctx, cell_layer, coaster->name, type, NULL);
    }
  } else if (s_state == STATE_RIDE_HISTORY) {
    RideRecord *ride = ride_data_get(cell_index->row);
    if (ride) {
      uint16_t max_whole = ride->max_g / 1000;
      uint16_t max_frac = (ride->max_g % 1000) / 10;
      snprintf(buffer, sizeof(buffer), "%s - Max: %u.%02ug", ride->coaster, max_whole, max_frac);
      menu_cell_basic_draw(ctx, cell_layer, buffer, NULL, NULL);
    }
  }
}

static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  if (s_state == STATE_PARK_SELECT) {
    s_park_idx = cell_index->row;
    enter_state(STATE_COASTER_SELECT);
  } else if (s_state == STATE_COASTER_SELECT) {
    s_coaster_idx = cell_index->row;
    enter_state(STATE_RECORDING);
  } else if (s_state == STATE_RIDE_HISTORY) {
    s_history_idx = cell_index->row;
  }
}

// ===== Recording Screen =====
static void recording_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  graphics_context_set_text_color(ctx, GColorWhite);
  
  char buf[64];
  ForceReading reading = forces_recorder_get_current();
  uint32_t elapsed = forces_recorder_get_elapsed_s();
  uint16_t max_g = forces_recorder_get_max_g();
  uint16_t min_g = forces_recorder_get_min_g();
  
  // Coaster name
  Coaster *coaster = parks_get_coaster(s_park_idx, s_coaster_idx);
  if (coaster) {
    graphics_draw_text(ctx, coaster->name, 
                       fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                       GRect(0, 10, bounds.size.w, 25),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  }
  
  // Elapsed time (large)
  snprintf(buf, sizeof(buf), "%02lu:%02lu", (unsigned long)(elapsed / 60), (unsigned long)(elapsed % 60));
  graphics_draw_text(ctx, buf, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD),
                     GRect(0, 38, bounds.size.w, 50),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  
  // Current G-force
  graphics_context_set_text_color(ctx, GColorLimerick);
  uint16_t g_whole = reading.magnitude / 1000;
  uint16_t g_frac = (reading.magnitude % 1000) / 10;
  snprintf(buf, sizeof(buf), "%u.%02u g", g_whole, g_frac);
  graphics_draw_text(ctx, buf, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD),
                     GRect(0, 88, bounds.size.w, 35),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  
  graphics_context_set_text_color(ctx, GColorWhite);
  
  // Max/Min
  uint16_t max_whole = max_g / 1000;
  uint16_t max_frac = (max_g % 1000) / 10;
  snprintf(buf, sizeof(buf), "Max: %u.%02u", max_whole, max_frac);
  graphics_draw_text(ctx, buf, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     GRect(5, 125, bounds.size.w / 2 - 5, 20),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  
  uint16_t min_whole = min_g / 1000;
  uint16_t min_frac = (min_g % 1000) / 10;
  snprintf(buf, sizeof(buf), "Min: %u.%02u", min_whole, min_frac);
  graphics_draw_text(ctx, buf, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     GRect(bounds.size.w / 2, 125, bounds.size.w / 2 - 5, 20),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);
  
  // Status text at bottom
  graphics_context_set_text_color(ctx, GColorDarkGray);
  const char *status = forces_recorder_is_recording() ? "Recording..." : "Paused";
  graphics_draw_text(ctx, status, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     GRect(0, bounds.size.h - 25, bounds.size.w, 20),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
}

static void recording_timer_callback(void *data) {
  if (s_recording_layer) {
    layer_mark_dirty(s_recording_layer);
  }
  if (s_recording_window) {
    s_recording_timer = app_timer_register(100, recording_timer_callback, NULL);
  }
}

static void recording_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  s_recording_layer = layer_create(bounds);
  layer_set_update_proc(s_recording_layer, recording_update_proc);
  layer_add_child(window_layer, s_recording_layer);
  
  s_recording_timer = app_timer_register(100, recording_timer_callback, NULL);
}

static void recording_window_unload(Window *window) {
  if (s_recording_timer) {
    app_timer_cancel(s_recording_timer);
    s_recording_timer = NULL;
  }
  if (s_recording_layer) {
    layer_destroy(s_recording_layer);
    s_recording_layer = NULL;
  }
}

// ===== State Management =====
static void show_recording_screen(void) {
  if (!s_recording_window) {
    s_recording_window = window_create();
    window_set_window_handlers(s_recording_window, (WindowHandlers) {
      .load = recording_window_load,
      .unload = recording_window_unload,
    });
  }
  window_stack_push(s_recording_window, true);
}

static void hide_recording_screen(void) {
  if (s_recording_window) {
    window_stack_remove(s_recording_window, true);
  }
}

static void return_to_previous_state(void) {
  if (s_state == STATE_COASTER_SELECT) {
    enter_state(STATE_PARK_SELECT);
  } else if (s_state == STATE_RIDE_HISTORY) {
    enter_state(STATE_PARK_SELECT);
  }
}

static void enter_state(AppState new_state) {
  // If coming from recording, save the ride
  if (s_state == STATE_RECORDING) {
    hide_recording_screen();
    forces_recorder_stop();
    
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
  }
  
  s_state = new_state;
  
  if (new_state == STATE_PARK_SELECT) {
    s_park_idx = 0;
    menu_layer_set_selected_index(s_menu_layer, (MenuIndex){0, 0}, MenuRowAlignCenter, false);
  } else if (new_state == STATE_COASTER_SELECT) {
    s_coaster_idx = 0;
    menu_layer_set_selected_index(s_menu_layer, (MenuIndex){0, 0}, MenuRowAlignCenter, false);
  } else if (new_state == STATE_RECORDING) {
    forces_recorder_start();
    show_recording_screen();
    return;  // Don't update menu layer for recording state
  } else if (new_state == STATE_RIDE_HISTORY) {
    s_history_idx = 0;
    menu_layer_set_selected_index(s_menu_layer, (MenuIndex){0, 0}, MenuRowAlignCenter, false);
  }
  
  menu_layer_reload_data(s_menu_layer);
}


// ===== Button Handlers =====
static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_state == STATE_PARK_SELECT || s_state == STATE_COASTER_SELECT || s_state == STATE_RIDE_HISTORY) {
    MenuIndex index = menu_layer_get_selected_index(s_menu_layer);
    menu_select_callback(s_menu_layer, &index, NULL);
  }
}

static void back_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_state == STATE_COASTER_SELECT || s_state == STATE_RIDE_HISTORY) {
    return_to_previous_state();
  } else if (s_state == STATE_RECORDING) {
    // Save and exit recording
    hide_recording_screen();
    forces_recorder_stop();
    
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
    
    enter_state(STATE_COASTER_SELECT);
  }
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_BACK, back_click_handler);
}

// ===== Window Handlers =====
static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  s_menu_layer = menu_layer_create(bounds);
  menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks) {
    .get_num_sections = menu_get_num_sections_callback,
    .get_num_rows = menu_get_num_rows_callback,
    .get_header_height = menu_get_header_height_callback,
    .draw_header = menu_draw_header_callback,
    .draw_row = menu_draw_row_callback,
    .select_click = menu_select_callback,
  });
  
  menu_layer_set_click_config_onto_window(s_menu_layer, window);
  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
  
  // Initialize app
  parks_init();
  ride_data_init();
  forces_recorder_init();
}

static void window_unload(Window *window) {
  menu_layer_destroy(s_menu_layer);
  s_menu_layer = NULL;
  
  forces_recorder_deinit();
  ride_data_deinit();
  parks_deinit();
}

// ===== Main App =====
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
  if (s_recording_window) {
    window_destroy(s_recording_window);
  }
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
