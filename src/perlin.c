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

static AppSync sync;
static uint8_t sync_buffer[256];

static int showdate;
static int bluetoothvibe;
static int hourlyvibe;
static int showbatt;
static int randomtime;
static int showsteps;
static int maxsteps;

static bool appStarted = false;

enum {
  DATE_KEY = 0x0,
  BLUETOOTHVIBE_KEY = 0x1,
  HOURLYVIBE_KEY = 0x2,
  BATT_KEY = 0x3,
  RANDOMTIME_KEY = 0x4,
  STEPS_KEY = 0x5,
  MAXSTEPS_KEY = 0x6
};

Window *window;
static Layer *window_layer;

TextLayer *layer_date_text;
TextLayer *layer_time_hour_text;
TextLayer *layer_time_min_text;

static GFont time_font;
static GFont date_font;

static GBitmap *background_image;
static BitmapLayer *background_layer;
int cur_day = -1;

int charge_percent = 0;

TextLayer *battery_text_layer;

static Layer *steps_layer;
static int current_steps = 0;

static int s_random = 20;
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

#ifdef PBL_HEALTH
static void update_steps(void) {
  time_t start = time_start_of_today();
  time_t end = time(NULL);

  HealthServiceAccessibilityMask mask =
      health_service_metric_accessible(HealthMetricStepCount, start, end);

  if (mask & HealthServiceAccessibilityMaskAvailable) {
    current_steps = (int)health_service_sum_today(HealthMetricStepCount);
  } else {
    current_steps = 0;
  }

  if (steps_layer) {
    layer_mark_dirty(steps_layer);
  }
}

static void health_handler(HealthEventType event, void *context) {
  if (event == HealthEventSignificantUpdate || event == HealthEventMovementUpdate) {
    update_steps();
  }
}
#else
static void update_steps(void) {
  current_steps = 0;

  if (steps_layer) {
    layer_mark_dirty(steps_layer);
  }
}
#endif


void theme_choice() {	
		
			if(s_random == 19){
			s_random = 0;
		} else {

			temp_random = rand() % 19;

			while(temp_random == s_random){
			    temp_random = rand() % 19;
		    }

		    s_random = temp_random;

	    if (background_image) {
		gbitmap_destroy(background_image);
		background_image = NULL;
    }

		   if(s_random == 0){
			   background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BG1);
         } else if(s_random == 1){
				background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BG2);
         } else if(s_random == 2){
				background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BG3);
         } else if(s_random == 3){
				background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BG4);
         } else if(s_random == 4){
				background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BG5);
         } else if(s_random == 5){
				background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BG6);
         } else if(s_random == 6){
 				background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BG7);
         } else if(s_random == 7){
 				background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BG8);
         } else if(s_random == 8){
 				background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BG9);
         } else if(s_random == 9){
 				background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BG10);
         } else if(s_random == 10){
 				background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BG11);
         } else if(s_random == 11){
 				background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BG12);
         } else if(s_random == 12){
 				background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BG13);
         } else if(s_random == 13){
 				background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BG14);
         } else if(s_random == 14){
 				background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BG15);
         } else if(s_random == 15){
 				background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BG16);
         } else if(s_random == 16){
 				background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BG17);
         } else if(s_random == 17){
 				background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BG18);
         } else if(s_random == 18){
 				background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BG19);
         } else if(s_random == 19){
 				background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BG20);
		 }

	   if (background_image != NULL) {
		bitmap_layer_set_bitmap(background_layer, background_image);
		layer_set_hidden(bitmap_layer_get_layer(background_layer), false);
		layer_mark_dirty(bitmap_layer_get_layer(background_layer));
    }		
  }
}

static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
  switch (key) {
		  
    case DATE_KEY:
      showdate = new_tuple->value->uint8 != 0;
	  persist_write_bool(DATE_KEY, showdate);
	  
	  if (showdate) {
		 layer_set_hidden(text_layer_get_layer(layer_date_text), true); 
	  } else {
		 layer_set_hidden(text_layer_get_layer(layer_date_text), false); 
	  }
	  break;
    
	case BLUETOOTHVIBE_KEY:
      bluetoothvibe = new_tuple->value->uint8 != 0;
	  persist_write_bool(BLUETOOTHVIBE_KEY, bluetoothvibe);
    break;     
     
	case HOURLYVIBE_KEY:
      hourlyvibe = new_tuple->value->uint8 != 0;
	  persist_write_bool(HOURLYVIBE_KEY, hourlyvibe);	  
    break;
	  
	  
	  case BATT_KEY:
		showbatt = new_tuple->value->uint8  !=0;
		persist_write_bool(BATT_KEY, showbatt);

	  if (showbatt) {
		 layer_set_hidden(text_layer_get_layer(battery_text_layer), true); 
	  } else {
		 layer_set_hidden(text_layer_get_layer(battery_text_layer), false); 
	  }
	  break;
	  
    case RANDOMTIME_KEY:
      randomtime = new_tuple->value->uint8 != 0;
	  persist_write_bool(RANDOMTIME_KEY, randomtime);	  

	  if (randomtime) {

	theme_choice();

	  }

     break;

	case STEPS_KEY:
	  showsteps = new_tuple->value->uint8 != 0;
	  persist_write_bool(STEPS_KEY, showsteps);

	  layer_set_hidden(steps_layer, !showsteps);
	  break;

	case MAXSTEPS_KEY:
	  maxsteps = new_tuple->value->int32;
	  persist_write_int(MAXSTEPS_KEY, maxsteps);

	  layer_mark_dirty(steps_layer);
	  break;
  }
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
	
if (units_changed & HOUR_UNIT) {
    hourvibe(tick_time);
	theme_choice();
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

	const int inbound_size = 256;
    const int outbound_size = 256;
    app_message_open(inbound_size, outbound_size);  
	
    window = window_create();
    window_stack_push(window, true);
 
    window_layer = window_get_root_layer(window);
	
  background_image = gbitmap_create_with_resource( RESOURCE_ID_IMAGE_BG1 );
  background_layer = bitmap_layer_create( layer_get_frame( window_layer ) );
  bitmap_layer_set_bitmap( background_layer, background_image );
  layer_add_child( window_layer, bitmap_layer_get_layer( background_layer ) );
	
	// resources

	time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_LECO_62));
	date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_LECO_14));

    // layer position and alignment
#if defined(PBL_PLATFORM_EMERY)
    // Pebble Time 2 (emery): 200x228 screen, larger frames for time and date
    layer_time_hour_text = text_layer_create(GRect(0, 24, 200, 70));
	layer_time_min_text = text_layer_create(GRect(0, 100, 200, 70));

    layer_date_text = text_layer_create(GRect(30, 198, 140, 22));
    battery_text_layer = text_layer_create(GRect(70, 6, 60, 20));

	steps_layer = layer_create(GRect(0, 182, 200, 8));

#elif defined(PBL_PLATFORM_CHALK)
    layer_time_hour_text = text_layer_create(GRect(0, 16, 182, 64));
	layer_time_min_text = text_layer_create(GRect(0, 73, 182, 64));

    layer_date_text = text_layer_create(GRect(0, 141, 178, 18));
    battery_text_layer = text_layer_create(GRect(0, 11, 178, 18));

	steps_layer = layer_create(GRect(0, 137, 180, 4));

#else
    layer_time_hour_text = text_layer_create(GRect(0, 15, 146, 64));
	layer_time_min_text = text_layer_create(GRect(0, 72, 146, 64));

    layer_date_text = text_layer_create(GRect(22, 145, 106, 18));
    battery_text_layer = text_layer_create(GRect(50, 5, 36, 18));

	steps_layer = layer_create(GRect(0, 138, 144, 5));

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
    text_layer_set_font(battery_text_layer, date_font);
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

#ifdef PBL_HEALTH
    health_service_events_subscribe(health_handler, NULL);
#endif

	showsteps = persist_exists(STEPS_KEY) ? persist_read_bool(STEPS_KEY) : 1;
	maxsteps = persist_exists(MAXSTEPS_KEY) ? persist_read_int(MAXSTEPS_KEY) : 10000;

   Tuplet initial_values[] = {
    TupletInteger(DATE_KEY, persist_read_bool(DATE_KEY)),
    TupletInteger(BLUETOOTHVIBE_KEY, persist_read_bool(BLUETOOTHVIBE_KEY)),
    TupletInteger(HOURLYVIBE_KEY, persist_read_bool(HOURLYVIBE_KEY)),
    TupletInteger(BATT_KEY, persist_read_bool(BATT_KEY)),
    TupletInteger(RANDOMTIME_KEY, persist_read_bool(RANDOMTIME_KEY)),
    TupletInteger(STEPS_KEY, showsteps),
    TupletInteger(MAXSTEPS_KEY, (int32_t)maxsteps),
  };
  
    app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values, ARRAY_LENGTH(initial_values),
      sync_tuple_changed_callback, NULL, NULL);
   
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
  app_sync_deinit(&sync);

  tick_timer_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
  battery_state_service_unsubscribe();

#ifdef PBL_HEALTH
  health_service_events_unsubscribe();
#endif

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
	
  window_destroy(window);

}

int main(void) {
    handle_init();
    app_event_loop();
    handle_deinit();
}
