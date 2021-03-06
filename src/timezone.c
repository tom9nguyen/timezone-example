#include <pebble.h>

// UI Elements
static Window *s_main_window = NULL;
static TextLayer *s_localtime_layer = NULL;
static TextLayer *s_localdate_layer = NULL;
static TextLayer *s_timezone_abbr_layer = NULL;
static TextLayer *s_region_layer = NULL;
static TextLayer *s_utctime_layer = NULL;
static TextLayer *s_utcdate_layer = NULL;
static TextLayer *s_timezone_utc_layer = NULL;

// Local time, Region and UTC display string buffers
static const char *s_timezone_abbr_string = NULL;
static char s_region_string[32] = "";          // Must be 32 characters large for longest option
static char s_localdate_string[16];
static char s_utcdate_string[16];
static char s_localtime_string[] = "00:00   "; // Leave space for 'AM/PM'
static char s_utctime_string[] = "00:00   ";   // Leave space for 'AM/PM'

// AM-PM or 24 hour clock
static bool s_is_clock_24 = false;

// Days of the week strings
static const char *const s_day_names[7] =
{"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  time_t current_time = time(NULL);
  struct tm *local_tm = localtime(&current_time);
  struct tm *utc_tm = gmtime(&current_time);

  // We don't check units_changed for DAY_UNIT as we are supporting 2 timezones,
  // and only localtime triggers a DAY_UNIT change.

  // Update local date
  snprintf(s_localdate_string, sizeof(s_localdate_string), "%s %d/%d",
    s_day_names[local_tm->tm_wday], local_tm->tm_mon + 1, local_tm->tm_mday);
  layer_mark_dirty(text_layer_get_layer(s_localdate_layer));

  // Update local time
  clock_copy_time_string(s_localtime_string,sizeof(s_localtime_string));
  layer_mark_dirty(text_layer_get_layer(s_localtime_layer));

  // Update UTC time and timezones only if a timezone has been set
  if (clock_is_timezone_set()) {
    // Show UTC layer if in UTC mode
    layer_set_hidden(text_layer_get_layer(s_timezone_utc_layer), false);
    layer_mark_dirty(text_layer_get_layer(s_timezone_utc_layer));

    // Update UTC time
    snprintf(s_utcdate_string, sizeof(s_utcdate_string), "%s %d/%d",
        s_day_names[utc_tm->tm_wday], utc_tm->tm_mon + 1, utc_tm->tm_mday);
    layer_mark_dirty(text_layer_get_layer(s_utcdate_layer));

    // Update timezone
    s_timezone_abbr_string = local_tm->tm_zone;
    text_layer_set_text(s_timezone_abbr_layer, s_timezone_abbr_string);
    layer_mark_dirty(text_layer_get_layer(s_timezone_abbr_layer));

    // Manually format utc time, to handle 24_hour or AM/PM modes
    snprintf(s_utctime_string, sizeof(s_utctime_string), "%d:%02d%s",
        (s_is_clock_24) ? utc_tm->tm_hour :
        ((utc_tm->tm_hour % 12 == 0) ? 12 : utc_tm->tm_hour % 12),
        utc_tm->tm_min,
        (s_is_clock_24) ? "" : ((utc_tm->tm_hour <= 12) ? " AM" : " PM"));
    layer_set_hidden(text_layer_get_layer(s_utctime_layer),false);
    layer_mark_dirty(text_layer_get_layer(s_utctime_layer));

    // Update timezone region
    clock_get_timezone(s_region_string, TIMEZONE_NAME_LENGTH);
    layer_set_hidden(text_layer_get_layer(s_region_layer),false);
    layer_mark_dirty(text_layer_get_layer(s_region_layer));
  }
}

/**
 * Convenience function to set up many TextLayers
 */
static void setup_text_layer(TextLayer **text_layer,
    const char *text_string, char *font_id, GRect pos) {
  *text_layer = text_layer_create(pos);
  text_layer_set_text(*text_layer, text_string);
	text_layer_set_font(*text_layer, fonts_get_system_font(font_id));
  text_layer_set_text_alignment(*text_layer, GTextAlignmentCenter);
  text_layer_set_background_color(*text_layer, GColorClear);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Timezone text
  setup_text_layer(&s_timezone_abbr_layer, s_timezone_abbr_string, FONT_KEY_GOTHIC_28_BOLD,
    grect_inset(bounds, GEdgeInsets(0)));
  layer_add_child(window_layer, text_layer_get_layer(s_timezone_abbr_layer));

  // Date text
  setup_text_layer(&s_localdate_layer, s_localdate_string, FONT_KEY_GOTHIC_18_BOLD,
    grect_inset(bounds, GEdgeInsets(26, 0, 0, 0)));
  layer_add_child(window_layer, text_layer_get_layer(s_localdate_layer));

  // Time text
  setup_text_layer(&s_localtime_layer, s_localtime_string, FONT_KEY_GOTHIC_28_BOLD,
    grect_inset(bounds, GEdgeInsets(38, 0, 0, 0)));
  layer_add_child(window_layer, text_layer_get_layer(s_localtime_layer));

  // Region text
  setup_text_layer(&s_region_layer, s_region_string, FONT_KEY_GOTHIC_18_BOLD,
    grect_inset(bounds, GEdgeInsets(72, 0, 0, 0)));
  layer_add_child(window_layer, text_layer_get_layer(s_region_layer));
  layer_set_hidden(text_layer_get_layer(s_region_layer), true);

  // UTC text
  setup_text_layer(&s_timezone_utc_layer, "UTC", FONT_KEY_GOTHIC_28_BOLD,
    grect_inset(bounds, GEdgeInsets(94, 0, 0, 0)));
  layer_add_child(window_layer, text_layer_get_layer(s_timezone_utc_layer));
  layer_set_hidden(text_layer_get_layer(s_timezone_utc_layer), true);

  // UTC date text
  setup_text_layer(&s_utcdate_layer, s_utcdate_string, FONT_KEY_GOTHIC_18_BOLD,
    grect_inset(bounds, GEdgeInsets(120, 0, 0, 0)));
  layer_add_child(window_layer, text_layer_get_layer(s_utcdate_layer));

  // UTC time text
  setup_text_layer(&s_utctime_layer, s_utctime_string, FONT_KEY_GOTHIC_28_BOLD,
    grect_inset(bounds, GEdgeInsets(132, 0, 0, 0)));
  layer_add_child(window_layer, text_layer_get_layer(s_utctime_layer));
  layer_set_hidden(text_layer_get_layer(s_utctime_layer), true);

  // Setup tick time handler
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);

  // Call handler once to populate initial time display
  time_t current_time = time(NULL);
  tick_handler(localtime(&current_time), SECOND_UNIT | MINUTE_UNIT | DAY_UNIT);
}

static void window_unload(Window *window) {
  // Nothing yet

}

static void handle_init(void) {
  // Get user's clock preference
  s_is_clock_24 = clock_is_24h_style();

  // Set up main Window
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorWhite);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });

  window_stack_push(s_main_window, false);
}

static void handle_deinit(void) {
  // Destroy main Window
  window_destroy(s_main_window);
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();

  return 0;
}

