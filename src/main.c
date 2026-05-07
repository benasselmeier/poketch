#include <pebble.h>

#define SHOW_SIDE_APP_BUTTONS 0
#define PALETTE_COUNT 8

static Window *s_window;
static Layer *s_layer;
static AppTimer *s_kitchen_timer;
static AppTimer *s_coin_timer;

static GBitmap *s_color_bar_bitmap;
static GBitmap *s_kecleon_bitmap;

static GRect s_status_bar_rect;
static GRect s_viewport_rect;
static GRect s_softkey_dock_rect;
static GRect s_softkey_rects[3];

#if defined(PBL_PLATFORM_EMERY) && SHOW_SIDE_APP_BUTTONS
static GRect s_prev_app_button_rect;
static GRect s_next_app_button_rect;
#endif

#if defined(PBL_TOUCH)
static uint32_t s_last_touch_action_ms = 0;
#endif

static int s_counter = 0;
static int s_steps = 6190;
static bool s_coin_heads = true;
static bool s_coin_flipping = false;
static int s_music_track_index = 0;
static bool s_music_playing = true;

static int s_timer_remaining_s = 5 * 60;
static bool s_timer_running = false;

static int s_active_palette = 0;
static int s_preview_palette = 0;

typedef struct {
  const char *name;
  GColor bg;
  GColor light;
  GColor mid;
  GColor dark;
} PoketchPalette;

typedef enum {
typedef enum {
  APP_MUSIC = 0,
  APP_COUNTER = 1,
  APP_COIN = 2,
  APP_PEDOMETER = 3,
  APP_KITCHEN_TIMER = 4,
  APP_KECLEON_THEME = 5,
  APP_COUNT = 6,
} PoketchApp;

typedef enum {
  SOFTKEY_LEFT = 0,
  SOFTKEY_MIDDLE = 1,
  SOFTKEY_RIGHT = 2,
} SoftKey;

static PoketchApp s_active_app = APP_MUSIC;

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
  switch (s_active_app) {
    case APP_MUSIC: return "Poketch - Music";
    case APP_COUNTER: return "Poketch - Counter";
    case APP_COIN: return "Poketch - Coin Flip";
    case APP_PEDOMETER: return "Poketch - Pedometer";
    case APP_KITCHEN_TIMER: return "Poketch - Timer";
    case APP_KECLEON_THEME: return "Poketch - Kecleon";
    default: return "Poketch";
  }
}

static const char *softkey_label(SoftKey key) {
  switch (s_active_app) {
    case APP_MUSIC:
      if (key == SOFTKEY_MIDDLE) return s_music_playing ? "Pause" : "Play";
      if (key == SOFTKEY_LEFT) return "Prev";
      return "Next";

    case APP_COUNTER:
      if (key == SOFTKEY_LEFT) return "Reset";
      if (key == SOFTKEY_MIDDLE) return "-1";
      return "+1";

    case APP_COIN:
      if (key == SOFTKEY_MIDDLE) return s_coin_flipping ? "..." : "Flip";
      return "";

    case APP_PEDOMETER:
      if (key == SOFTKEY_LEFT) return "Reset";
      if (key == SOFTKEY_MIDDLE) return "-10";
      return "+10";

    case APP_KITCHEN_TIMER:
      if (key == SOFTKEY_LEFT) return "Reset";
      if (key == SOFTKEY_MIDDLE) return s_timer_running ? "Stop" : "Start";
      return "+1m";

    case APP_KECLEON_THEME:
      if (key == SOFTKEY_LEFT) return "Prev";
      if (key == SOFTKEY_MIDDLE) return "Apply";
      return "Next";

    default:
      return "";
  }
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

typedef struct {
  const char *title;
  const char *artist;
  const char *album;
  const char *time_text;
} MusicTrack;

static const MusicTrack s_music_tracks[] = {
  {"The Peak of Superstition", "Dance Gavin Dance", "Pantheon", "00:43 / 04:11"},
  {"Feather", "Nujabes", "Modal Soul", "01:22 / 05:32"},
  {"Deadbolt", "Thrice", "The Artist in the Ambulance", "02:09 / 03:00"},
};

static const int s_music_track_count = sizeof(s_music_tracks) / sizeof(s_music_tracks[0]);

static void music_prev_track(void) {
  s_music_track_index--;
  if (s_music_track_index < 0) s_music_track_index = s_music_track_count - 1;
  redraw();
}

static void music_next_track(void) {
  s_music_track_index = (s_music_track_index + 1) % s_music_track_count;
  redraw();
}

static void music_toggle_play_pause(void) {
  s_music_playing = !s_music_playing;
  redraw();
}

static void kitchen_timer_tick(void *context) {
  if (!s_timer_running) return;

  if (s_timer_remaining_s > 0) s_timer_remaining_s--;

  if (s_timer_remaining_s > 0) {
    s_kitchen_timer = app_timer_register(1000, kitchen_timer_tick, NULL);
  } else {
    s_timer_running = false;
    s_kitchen_timer = NULL;
    vibes_short_pulse();
  }

  redraw();
}

static void toggle_kitchen_timer(void) {
  s_timer_running = !s_timer_running;

  if (s_timer_running) {
    s_kitchen_timer = app_timer_register(1000, kitchen_timer_tick, NULL);
  } else if (s_kitchen_timer) {
    app_timer_cancel(s_kitchen_timer);
    s_kitchen_timer = NULL;
  }

  redraw();
}

static void reset_kitchen_timer(void) {
  s_timer_running = false;
  s_timer_remaining_s = 5 * 60;

  if (s_kitchen_timer) {
    app_timer_cancel(s_kitchen_timer);
    s_kitchen_timer = NULL;
  }

  redraw();
}

static void next_app(void) {
  s_active_app = (s_active_app + 1) % APP_COUNT;

  if (s_active_app == APP_KECLEON_THEME) {
    s_preview_palette = s_active_palette;
  }

  redraw();
}

static void prev_app(void) {
  int next = (int)s_active_app - 1;
  if (next < 0) next = APP_COUNT - 1;

  s_active_app = next;

  if (s_active_app == APP_KECLEON_THEME) {
    s_preview_palette = s_active_palette;
  }

  redraw();
}

static void run_softkey(SoftKey key) {
  switch (s_active_app) {
    case APP_MUSIC:
      if (key == SOFTKEY_LEFT) music_prev_track();
      else if (key == SOFTKEY_MIDDLE) music_toggle_play_pause();
      else if (key == SOFTKEY_RIGHT) music_next_track();
      break;
    case APP_COUNTER:
      if (key == SOFTKEY_LEFT) s_counter = 0;
      else if (key == SOFTKEY_MIDDLE && s_counter > 0) s_counter--;
      else if (key == SOFTKEY_RIGHT) s_counter++;
      break;

    case APP_COIN:
      if (key == SOFTKEY_MIDDLE) flip_coin();
      break;

    case APP_PEDOMETER:
      if (key == SOFTKEY_LEFT) s_steps = 0;
      else if (key == SOFTKEY_MIDDLE) {
        s_steps -= 10;
        if (s_steps < 0) s_steps = 0;
      } else if (key == SOFTKEY_RIGHT) {
        s_steps += 10;
      }
      break;

    case APP_KITCHEN_TIMER:
      if (key == SOFTKEY_LEFT) reset_kitchen_timer();
      else if (key == SOFTKEY_MIDDLE) toggle_kitchen_timer();
      else if (key == SOFTKEY_RIGHT) s_timer_remaining_s += 60;
      break;

    case APP_KECLEON_THEME:
      if (key == SOFTKEY_LEFT) {
        s_preview_palette--;
        if (s_preview_palette < 0) s_preview_palette = PALETTE_COUNT - 1;
      } else if (key == SOFTKEY_RIGHT) {
        s_preview_palette++;
        if (s_preview_palette >= PALETTE_COUNT) s_preview_palette = 0;
      } else if (key == SOFTKEY_MIDDLE) {
        s_active_palette = s_preview_palette;
        vibes_short_pulse();
      }
      break;

    default:
      break;
  }

  redraw();
}

static void run_primary_action(void) {
  if (s_active_app == APP_COIN) {
    flip_coin();
  } else if (s_active_app == APP_MUSIC) {
    music_toggle_play_pause();
  } else if (s_active_app == APP_KECLEON_THEME) {
    run_softkey(SOFTKEY_MIDDLE);
  } else {
    run_softkey(SOFTKEY_RIGHT);
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

static void draw_side_buttons(GContext *ctx) {
#if defined(PBL_PLATFORM_EMERY) && SHOW_SIDE_APP_BUTTONS
  int red_w = s_viewport_rect.size.w / 6;
  if (red_w < 28) red_w = 28;
  if (red_w > 40) red_w = 40;

  int gap = 6;
  int total_h = s_viewport_rect.size.h - gap;
  int red_h = total_h / 2;

  int red_x = s_viewport_rect.origin.x +
              s_viewport_rect.size.w - red_w - 2;

  int red_top_y = s_viewport_rect.origin.y;
  int red_bottom_y = red_top_y + red_h + gap;

  s_prev_app_button_rect = GRect(red_x, red_top_y, red_w, red_h);
  s_next_app_button_rect = GRect(red_x, red_bottom_y, red_w, red_h);

  graphics_context_set_fill_color(ctx, GColorDarkGray);

  graphics_fill_rect(ctx, s_prev_app_button_rect, 10,
                     GCornerTopLeft | GCornerBottomLeft);

  graphics_fill_rect(ctx, s_next_app_button_rect, 10,
                     GCornerTopLeft | GCornerBottomLeft);

  GRect prev_inner = inset_rect(s_prev_app_button_rect, 2);
  GRect next_inner = inset_rect(s_next_app_button_rect, 2);

  graphics_context_set_fill_color(ctx, GColorRed);

  graphics_fill_rect(ctx, prev_inner, 8,
                     GCornerTopLeft | GCornerBottomLeft);

  graphics_fill_rect(ctx, next_inner, 8,
                     GCornerTopLeft | GCornerBottomLeft);
#endif
}

static void draw_kecleon_theme_app(GContext *ctx, GRect content) {
  PoketchPalette preview = palette_get(s_preview_palette);

  GRect preview_area = inset_rect(content, 4);

  graphics_context_set_fill_color(ctx, preview.bg);
  graphics_fill_rect(ctx, preview_area, 0, GCornerNone);

  graphics_context_set_stroke_color(ctx, preview.dark);
  graphics_draw_rect(ctx, preview_area);

  if (s_kecleon_bitmap) {
    graphics_draw_bitmap_in_rect(ctx,
                                 s_kecleon_bitmap,
                                 GRect(content.origin.x + 18,
                                       content.origin.y + 8,
                                       64,
                                       64));
  }

  if (s_color_bar_bitmap) {
    graphics_draw_bitmap_in_rect(ctx,
                                 s_color_bar_bitmap,
                                 GRect(content.origin.x + 18,
                                       content.origin.y + 75,
                                       120,
                                       20));
  }

  graphics_context_set_text_color(ctx, preview.dark);

  graphics_draw_text(ctx,
                     preview.name,
                     fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     GRect(content.origin.x,
                           content.origin.y + 96,
                           content.size.w,
                           22),
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentCenter,
                     NULL);

  if (s_preview_palette == s_active_palette) {
    graphics_draw_text(ctx,
                       "ACTIVE",
                       fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                       GRect(content.origin.x,
                             content.origin.y + 114,
                             content.size.w,
                             18),
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentCenter,
                       NULL);
  }
}

static void draw_app_content(GContext *ctx) {
  PoketchPalette palette = current_palette();

  GRect content = inset_rect(s_viewport_rect, 8);

#if defined(PBL_PLATFORM_EMERY) && SHOW_SIDE_APP_BUTTONS
  content.size.w -= 34;
#endif

  graphics_context_set_text_color(ctx, palette.dark);

  if (s_active_app == APP_COUNTER ||
      s_active_app == APP_PEDOMETER) {

    int value = (s_active_app == APP_COUNTER)
      ? s_counter
      : s_steps;

    char value_buf[16];
    snprintf(value_buf, sizeof(value_buf), "%d", value);

    graphics_draw_text(ctx,
                       value_buf,
                       fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD),
                       GRect(content.origin.x,
                             content.origin.y + 18,
                             content.size.w,
                             50),
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentCenter,
                       NULL);

    if (s_active_app == APP_PEDOMETER) {
      graphics_draw_text(ctx,
                         "STEPS",
                         fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD),
                         GRect(content.origin.x,
                               content.origin.y + 62,
                               content.size.w,
                               34),
                         GTextOverflowModeTrailingEllipsis,
                         GTextAlignmentCenter,
                         NULL);
    }
  }

  else if (s_active_app == APP_COIN) {
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
  } else if (s_active_app == APP_MUSIC) {
    const MusicTrack *track = &s_music_tracks[s_music_track_index];
    graphics_draw_text(ctx,
                       track->title,
                       fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
                       GRect(content.origin.x + 5, content.origin.y + 27, content.size.w - 10, 30),
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentLeft,
                       NULL);
    graphics_draw_text(ctx,
                       track->artist,
                       fonts_get_system_font(FONT_KEY_GOTHIC_24),
                       GRect(content.origin.x + 5, content.origin.y + 54, content.size.w - 10, 28),
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentLeft,
                       NULL);
    graphics_draw_text(ctx,
                       track->album,
                       fonts_get_system_font(FONT_KEY_GOTHIC_24_OBLIQUE),
                       GRect(content.origin.x + 5, content.origin.y + 79, content.size.w - 10, 28),
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentLeft,
                       NULL);
    graphics_draw_text(ctx,
                       track->time_text,
                       fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
                       GRect(content.origin.x + 2, content.origin.y + content.size.h - 32, content.size.w - 4, 26),
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentCenter,
                       NULL);
    graphics_draw_text(ctx,
                       s_music_playing ? "PLAYING" : "PAUSED",
                       fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(content.origin.x + 2, content.origin.y + 2, content.size.w - 4, 14),
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentRight,
                       NULL);
  }

  else if (s_active_app == APP_KITCHEN_TIMER) {
    int min = s_timer_remaining_s / 60;
    int sec = s_timer_remaining_s % 60;

    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d", min, sec);

    graphics_draw_text(ctx,
                       buf,
                       fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD),
                       content,
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentCenter,
                       NULL);
  }

  else if (s_active_app == APP_KECLEON_THEME) {
    draw_kecleon_theme_app(ctx, content);
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

#if defined(PBL_PLATFORM_EMERY) && SHOW_SIDE_APP_BUTTONS
  if (grect_contains_point(&s_prev_app_button_rect, &pt)) {
    prev_app();
    return;
  }

  if (grect_contains_point(&s_next_app_button_rect, &pt)) {
    next_app();
    return;
  }
#endif

  for (int i = 0; i < 3; i++) {
    if (grect_contains_point(&s_softkey_rects[i], &pt)) {
      run_softkey((SoftKey)i);
      return;
    }
  }
}
#endif

static void select_click_handler(ClickRecognizerRef r, void *c) {
  run_primary_action();
}

static void select_long_click_handler(ClickRecognizerRef r, void *c) {
  run_softkey(SOFTKEY_LEFT);
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_active_app == APP_MUSIC) {
    music_prev_track();
    return;
  }
  prev_app();
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_active_app == APP_MUSIC) {
    music_next_track();
    return;
  }
  next_app();
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

  s_color_bar_bitmap = gbitmap_create_with_resource(RESOURCE_ID_color_bar_WHITE);
  s_kecleon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_kecleon_WHITE);

#if defined(PBL_TOUCH)
  touch_service_subscribe(touch_handler, NULL);
#endif
}

static void window_unload(Window *window) {
#if defined(PBL_TOUCH)
  touch_service_unsubscribe();
#endif

  if (s_kitchen_timer) {
    app_timer_cancel(s_kitchen_timer);
    s_kitchen_timer = NULL;
  }

  if (s_coin_timer) {
    app_timer_cancel(s_coin_timer);
    s_coin_timer = NULL;
  }

  if (s_color_bar_bitmap) {
    gbitmap_destroy(s_color_bar_bitmap);
    s_color_bar_bitmap = NULL;
  }

  if (s_kecleon_bitmap) {
    gbitmap_destroy(s_kecleon_bitmap);
    s_kecleon_bitmap = NULL;
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