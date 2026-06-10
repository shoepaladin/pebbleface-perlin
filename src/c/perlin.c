/*
Copyright (C) 2015 Mark Reed

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), 
to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "pebble.h"

static int showdate;
static int bluetoothvibe;
static int hourlyvibe;
static int showbatt;
static int randomtime;
static int showsteps;
static int maxsteps;
static int refreshhours;
static int hours_since_refresh = 0;

static bool appStarted = false;

enum {
  DATE_KEY = 0x0,
  BLUETOOTHVIBE_KEY = 0x1,
  HOURLYVIBE_KEY = 0x2,
  BATT_KEY = 0x3,
  RANDOMTIME_KEY = 0x4,
  STEPS_KEY = 0x5,
  MAXSTEPS_KEY = 0x6,
  REFRESH_KEY = 0x7
};

Window *window;
static Layer *window_layer;

TextLayer *layer_date_text;
TextLayer *layer_time_hour_text;
TextLayer *layer_time_min_text;

static GFont time_font;
static GFont date_font;
static GFont batt_font;

static GBitmap *background_image;
static BitmapLayer *background_layer;
int cur_day = -1;

int charge_percent = 0;

TextLayer *battery_text_layer;

static Layer *steps_layer;
static int current_steps = 0;

static int s_random = 0;   // BG1 is loaded at init
static int temp_random;

static void steps_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  // bar grows equally left/right from the center, keeping 5px padding per side at max
  int max_width = bounds.size.w - 10;
  int max = maxsteps > 0 ? maxsteps : 10000;

  int steps = current_steps;
  if (steps > max) {
    steps = max;
  }

  int width = (max_width * steps) / max;
  if (width <= 0) {
    return;
  }

  GRect bar = GRect((bounds.size.w - width) / 2, 0, width, bounds.size.h);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bar, 1, GCornersAll);
}

// Steps are polled from the minute tick handler (and on launch) rather than
// relying on a HealthService event subscription.
static void update_steps(void) {
#if defined(PBL_HEALTH)
  time_t start = time_start_of_today();
  time_t end = time(NULL);

  HealthServiceAccessibilityMask mask =
      health_service_metric_accessible(HealthMetricStepCount, start, end);

  if (mask & HealthServiceAccessibilityMaskAvailable) {
    current_steps = (int)health_service_sum_today(HealthMetricStepCount);
  } else {
    current_steps = 0;
  }
#else
  current_steps = 0;   // watch has no health support
#endif

  if (steps_layer) {
    layer_mark_dirty(steps_layer);
  }
}


// 25 rotating backgrounds. BG21-BG25 are new procedurally generated
// perlin-noise themes (see tools/generate_backgrounds.py).
static const uint32_t BG_RESOURCES[] = {
  RESOURCE_ID_IMAGE_BG1,  RESOURCE_ID_IMAGE_BG2,  RESOURCE_ID_IMAGE_BG3,
  RESOURCE_ID_IMAGE_BG4,  RESOURCE_ID_IMAGE_BG5,  RESOURCE_ID_IMAGE_BG6,
  RESOURCE_ID_IMAGE_BG7,  RESOURCE_ID_IMAGE_BG8,  RESOURCE_ID_IMAGE_BG9,
  RESOURCE_ID_IMAGE_BG10, RESOURCE_ID_IMAGE_BG11, RESOURCE_ID_IMAGE_BG12,
  RESOURCE_ID_IMAGE_BG13, RESOURCE_ID_IMAGE_BG14, RESOURCE_ID_IMAGE_BG15,
  RESOURCE_ID_IMAGE_BG16, RESOURCE_ID_IMAGE_BG17, RESOURCE_ID_IMAGE_BG18,
  RESOURCE_ID_IMAGE_BG19, RESOURCE_ID_IMAGE_BG20, RESOURCE_ID_IMAGE_BG21,
  RESOURCE_ID_IMAGE_BG22, RESOURCE_ID_IMAGE_BG23, RESOURCE_ID_IMAGE_BG24,
  RESOURCE_ID_IMAGE_BG25
};

void theme_choice() {
  temp_random = rand() % (int)ARRAY_LENGTH(BG_RESOURCES);
  while (temp_random == s_random) {
    temp_random = rand() % (int)ARRAY_LENGTH(BG_RESOURCES);
  }
  s_random = temp_random;

  if (background_image) {
    gbitmap_destroy(background_image);
    background_image = NULL;
  }

  background_image = gbitmap_create_with_resource(BG_RESOURCES[s_random]);

  if (background_image != NULL) {
    bitmap_layer_set_bitmap(background_layer, background_image);
    layer_set_hidden(bitmap_layer_get_layer(background_layer), false);
    layer_mark_dirty(bitmap_layer_get_layer(background_layer));
  }
}

// Settings arrive from Clay as a plain AppMessage; read each key with
// dict_find and persist it. Toggles arrive as ints, the step goal input
// arrives as a CString.
static void in_received_handler(DictionaryIterator *iter, void *context) {
  Tuple *t;

  t = dict_find(iter, DATE_KEY);
  if (t) {
    showdate = t->value->int32 != 0;
    persist_write_bool(DATE_KEY, showdate);
    layer_set_hidden(text_layer_get_layer(layer_date_text), !showdate);
  }

  t = dict_find(iter, BLUETOOTHVIBE_KEY);
  if (t) {
    bluetoothvibe = t->value->int32 != 0;
    persist_write_bool(BLUETOOTHVIBE_KEY, bluetoothvibe);
  }

  t = dict_find(iter, HOURLYVIBE_KEY);
  if (t) {
    hourlyvibe = t->value->int32 != 0;
    persist_write_bool(HOURLYVIBE_KEY, hourlyvibe);
  }

  t = dict_find(iter, BATT_KEY);
  if (t) {
    showbatt = t->value->int32 != 0;
    persist_write_bool(BATT_KEY, showbatt);
    layer_set_hidden(text_layer_get_layer(battery_text_layer), !showbatt);
  }

  t = dict_find(iter, RANDOMTIME_KEY);
  if (t) {
    randomtime = t->value->int32 != 0;
    persist_write_bool(RANDOMTIME_KEY, randomtime);

    if (randomtime) {
      theme_choice();
    }
  }

  t = dict_find(iter, STEPS_KEY);
  if (t) {
    showsteps = t->value->int32 != 0;
    persist_write_bool(STEPS_KEY, showsteps);
    layer_set_hidden(steps_layer, !showsteps);
  }

  t = dict_find(iter, MAXSTEPS_KEY);
  if (t) {
    int v;
    // Clay sends the input field as a CString; tolerate either.
    if (t->type == TUPLE_CSTRING) {
      v = atoi(t->value->cstring);
    } else {
      v = (int)t->value->int32;
    }
    if (v < 100) v = 100;        // guardrails
    if (v > 100000) v = 100000;
    maxsteps = v;
    persist_write_int(MAXSTEPS_KEY, maxsteps);
    layer_mark_dirty(steps_layer);
  }

  t = dict_find(iter, REFRESH_KEY);
  if (t) {
    int v;
    // Clay sends radiogroup values as CStrings; tolerate either.
    if (t->type == TUPLE_CSTRING) {
      v = atoi(t->value->cstring);
    } else {
      v = (int)t->value->int32;
    }
    if (v < 1) v = 1;
    if (v > 60) v = 60;
    refreshhours = v;
    persist_write_int(REFRESH_KEY, refreshhours);
    hours_since_refresh = 0;   // restart the countdown from now
  }
}

static void load_persisted_settings(void) {
  showdate      = persist_exists(DATE_KEY) ? persist_read_bool(DATE_KEY) : 1;
  bluetoothvibe = persist_exists(BLUETOOTHVIBE_KEY) ? persist_read_bool(BLUETOOTHVIBE_KEY) : 1;
  hourlyvibe    = persist_exists(HOURLYVIBE_KEY) ? persist_read_bool(HOURLYVIBE_KEY) : 0;
  showbatt      = persist_exists(BATT_KEY) ? persist_read_bool(BATT_KEY) : 1;
  randomtime    = persist_exists(RANDOMTIME_KEY) ? persist_read_bool(RANDOMTIME_KEY) : 0;
  showsteps     = persist_exists(STEPS_KEY) ? persist_read_bool(STEPS_KEY) : 1;
  maxsteps      = persist_exists(MAXSTEPS_KEY) ? persist_read_int(MAXSTEPS_KEY) : 10000;
  if (maxsteps < 100) maxsteps = 10000;
  refreshhours  = persist_exists(REFRESH_KEY) ? persist_read_int(REFRESH_KEY) : 1;
  if (refreshhours < 1 || refreshhours > 60) refreshhours = 1;
}

void update_battery_state(BatteryChargeState charge_state) {
    static char battery_text[] = "x100";

    if (charge_state.is_charging) {

        snprintf(battery_text, sizeof(battery_text), "+%d", charge_state.charge_percent);
    } else {
        snprintf(battery_text, sizeof(battery_text), "%d", charge_state.charge_percent);
        
    } 
    charge_percent = charge_state.charge_percent;
    
    text_layer_set_text(battery_text_layer, battery_text);
	
} 

static void toggle_bluetooth(bool connected) {

if (appStarted && !connected && bluetoothvibe) {
	  
    //vibe!
    vibes_long_pulse();
   }
}

void bluetooth_connection_callback(bool connected) {
  toggle_bluetooth(connected);
}

void update_time(struct tm *tick_time) {

	static char h_time_text[] = "00";
	static char m_time_text[] = "00";

    static char date_text[] = "XXX XXX 00XX XXX";
   
    char *h_time_format;

    int new_cur_day = tick_time->tm_year*1000 + tick_time->tm_yday;
    if (new_cur_day != cur_day) {
        cur_day = new_cur_day;

	switch(tick_time->tm_mday)
  {
    case 1 :
    case 21 :
    case 31 :
      strftime(date_text, sizeof(date_text), "%a, %est %b", tick_time);
      break;
    case 2 :
    case 22 :
      strftime(date_text, sizeof(date_text), "%a, %end %b", tick_time);
      break;
    case 3 :
    case 23 :
      strftime(date_text, sizeof(date_text), "%a, %erd %b", tick_time);
      break;
    default :
      strftime(date_text, sizeof(date_text), "%a, %eth %b", tick_time);
      break;
  }
	  text_layer_set_text(layer_date_text, date_text);
}

   if (clock_is_24h_style()) {
        h_time_format = "%H";		
    } else {
        h_time_format = "%I";
	
   }
    strftime(h_time_text, sizeof(h_time_text), h_time_format, tick_time);
    text_layer_set_text(layer_time_hour_text, h_time_text);

	strftime(m_time_text, sizeof(m_time_text), "%M", tick_time);
	text_layer_set_text(layer_time_min_text, m_time_text);
}

void hourvibe (struct tm *tick_time) {

	  if(appStarted && hourlyvibe) {
    //vibe!
    vibes_short_pulse();
  }
}

void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
    update_time(tick_time);
    update_steps();

if (units_changed & HOUR_UNIT) {
    hourvibe(tick_time);

    // change the background every refreshhours hours
    hours_since_refresh++;
    if (hours_since_refresh >= refreshhours) {
      hours_since_refresh = 0;
      theme_choice();
    }
}
	
if (units_changed & MINUTE_UNIT) { 	
//theme_choice();  // used for testing	
 }
}

void force_update(void) {
    toggle_bluetooth(bluetooth_connection_service_peek());
    time_t now = time(NULL);
    update_time(localtime(&now));
}

void handle_init(void) {

	load_persisted_settings();

	app_message_register_inbox_received(in_received_handler);
    app_message_open(256, 64);

    window = window_create();
    window_stack_push(window, true);
 
    window_layer = window_get_root_layer(window);
	
  background_image = gbitmap_create_with_resource( RESOURCE_ID_IMAGE_BG1 );
  background_layer = bitmap_layer_create( layer_get_frame( window_layer ) );
  bitmap_layer_set_bitmap( background_layer, background_image );
  layer_add_child( window_layer, bitmap_layer_get_layer( background_layer ) );
	
	// resources

	time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_LECO_62));
	date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_LECO_20));
	batt_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_LECO_14));

    // layer position and alignment
#if defined(PBL_PLATFORM_EMERY)
    // Pebble Time 2 (emery): 200x228 screen, larger frames for time and date
    layer_time_hour_text = text_layer_create(GRect(0, 24, 200, 70));
	layer_time_min_text = text_layer_create(GRect(0, 100, 200, 70));

    layer_date_text = text_layer_create(GRect(20, 194, 160, 28));
    battery_text_layer = text_layer_create(GRect(70, 6, 60, 20));

	steps_layer = layer_create(GRect(0, 182, 200, 8));

#elif defined(PBL_PLATFORM_CHALK)
    layer_time_hour_text = text_layer_create(GRect(0, 16, 182, 64));
	layer_time_min_text = text_layer_create(GRect(0, 73, 182, 64));

    layer_date_text = text_layer_create(GRect(0, 143, 178, 28));
    battery_text_layer = text_layer_create(GRect(0, 11, 178, 18));

	steps_layer = layer_create(GRect(0, 138, 180, 4));

#else
    layer_time_hour_text = text_layer_create(GRect(0, 15, 146, 64));
	layer_time_min_text = text_layer_create(GRect(0, 72, 146, 64));

    layer_date_text = text_layer_create(GRect(8, 142, 128, 26));
    battery_text_layer = text_layer_create(GRect(50, 5, 36, 18));

	steps_layer = layer_create(GRect(0, 137, 144, 4));

#endif

    window_set_background_color(window, GColorWhite);
	
    text_layer_set_text_color(layer_time_hour_text, GColorBlack);
	text_layer_set_text_color(layer_date_text, GColorBlack);		
    text_layer_set_text_color(battery_text_layer, GColorBlack);
    text_layer_set_text_color(layer_time_min_text, GColorBlack);
	
	text_layer_set_background_color(layer_time_hour_text, GColorClear);
	text_layer_set_background_color(layer_date_text, GColorWhite);
	text_layer_set_background_color(battery_text_layer, GColorWhite);
	text_layer_set_background_color(layer_time_min_text, GColorClear);

    text_layer_set_font(layer_time_hour_text, time_font);
    text_layer_set_font(layer_date_text, date_font);
    text_layer_set_font(battery_text_layer, batt_font);
    text_layer_set_font(layer_time_min_text, time_font);

	text_layer_set_text_alignment(layer_time_hour_text, GTextAlignmentCenter);
    text_layer_set_text_alignment(layer_date_text, GTextAlignmentCenter);
    text_layer_set_text_alignment(battery_text_layer, GTextAlignmentCenter);
	text_layer_set_text_alignment(layer_time_min_text, GTextAlignmentCenter);

	layer_set_update_proc(steps_layer, steps_update_proc);

    // composing layers
    layer_add_child(window_layer, text_layer_get_layer(layer_time_hour_text));
    layer_add_child(window_layer, text_layer_get_layer(layer_date_text));
    layer_add_child(window_layer, text_layer_get_layer(battery_text_layer));
    layer_add_child(window_layer, text_layer_get_layer(layer_time_min_text));
    layer_add_child(window_layer, steps_layer);

     // handlers
    battery_state_service_subscribe(&update_battery_state);
    bluetooth_connection_service_subscribe(&toggle_bluetooth);
    tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);

	// apply persisted settings to the layers
	layer_set_hidden(text_layer_get_layer(layer_date_text), !showdate);
	layer_set_hidden(text_layer_get_layer(battery_text_layer), !showbatt);

    appStarted = true;
	
	// update the battery on launch
    update_battery_state(battery_state_service_peek());

	// initial step count for the progress bar
	update_steps();

	layer_set_hidden(steps_layer, !showsteps);

    // draw first frame
    force_update();
}

void handle_deinit(void) {
  app_message_deregister_callbacks();

  tick_timer_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
  battery_state_service_unsubscribe();

  layer_remove_from_parent(bitmap_layer_get_layer(background_layer));
  bitmap_layer_destroy(background_layer);
  gbitmap_destroy(background_image);
  background_image = NULL;
  
  text_layer_destroy( layer_time_hour_text );
  text_layer_destroy( layer_time_min_text );
  text_layer_destroy( layer_date_text );
  text_layer_destroy( battery_text_layer );

  layer_destroy( steps_layer );
	
  fonts_unload_custom_font(time_font);
  fonts_unload_custom_font(date_font);
  fonts_unload_custom_font(batt_font);
	
  window_destroy(window);

}

int main(void) {
    handle_init();
    app_event_loop();
    handle_deinit();
}
