#include <pebble.h>

static Window *window;
static TextLayer *text_layer;
static AppTimer *timer;
static int frequency = 10;

static void set_timer();
static void timer_handler();
static void display_state();


typedef enum state state;
enum state{
	IDLE = 0,
	READY = 1,
	LOGGING = 2
};

enum { 
	STAMP_S_KEY = 0,
	STAMP_MS_KEY = 1, 
	X_KEY = 2, 
	Y_KEY = 3, 
	Z_KEY = 4 
};

enum {
	STATE_KEY = 0
};

state current_state;

static void in_received_handler(DictionaryIterator* iter, void* context)
{
//	Tuple *state_tuple = dict_find(iter, STATE_KEY);
//	current_state = (state)state_tuple->value;
////	int new_state = (int)state_tuple->value;
////	switch (new_state) {
////		case 0: current_state = IDLE; break;
////		case 1: current_state = READY; break;
////		case 2: current_state = LOGGING; break;
////		default:
////			break;
////	}
//	
//	
//	//display current state
//	display_state();
}

void out_sent_handler(DictionaryIterator *sent, void *context) {
   // outgoing message was delivered
 }


 void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
   // outgoing message failed
 }

 void in_dropped_handler(AppMessageResult reason, void *context) {
   // incoming message dropped
 }

static void display_state()
{
	char* state_str = "";
	switch (current_state) {
		case IDLE: 
			state_str = "IDLE";
			break;
		case READY: 
			state_str = "BUMP TO RECORD";
			break;
		case LOGGING: 
			state_str = "LOGGING";
			break;
		default:
			break;
	}
	
	text_layer_set_text(text_layer, state_str); 
}

static void timer_handler()
{
//	text_layer_set_text(text_layer, "TIMER HANDLER");
	
	if (current_state == IDLE) return;
	
	AccelData current_sample = {.x = 0, .y = 0, .z = 0};
	accel_service_peek(&current_sample);
	
	if (current_state == READY && (current_sample.y > 1000 || current_sample.y < -1000)) {
		current_state = LOGGING;
//		text_layer_set_text(text_layer, "SWITCHED TO LOGGING");
	}
	
	if (current_state == LOGGING)
	{
		//display accel values on Pebble
		char* text = "";
		snprintf(text, 100, "%d", current_sample.x);
		text_layer_set_text(text_layer, text);
		
		time_t current_stamp_s;
		current_stamp_s = time(NULL);

		uint16_t current_stamp_ms;
		current_stamp_ms = time_ms(NULL, NULL);
		
		uint16_t current_stamp = current_stamp_s + 1000 * current_stamp_ms;
		
		APP_LOG(APP_LOG_LEVEL_DEBUG, "%lu", ((uint32_t)current_stamp_s * 1000 + current_stamp_ms));
		
		int x = current_sample.x;
		int y = current_sample.y;
		int z = current_sample.z;
		
//		APP_LOG(APP_LOG_LEVEL_DEBUG, "%d, %d, %d, %d", (int)current_stamp, x, y, z);
		
		//call app message to send x, y, z data to phone
		DictionaryIterator *iter;
		app_message_outbox_begin(&iter);

		Tuplet stampval_s = TupletInteger(STAMP_S_KEY, current_stamp_s);
		dict_write_tuplet(iter, &stampval_s);
		
		Tuplet stampval_ms = TupletInteger(STAMP_MS_KEY, current_stamp_ms);
		dict_write_tuplet(iter, &stampval_ms);
		
		Tuplet xval = TupletInteger(X_KEY, x);
		dict_write_tuplet(iter, &xval);
		
		Tuplet yval = TupletInteger(Y_KEY, y);
		dict_write_tuplet(iter, &yval);
		
		Tuplet zval = TupletInteger(Z_KEY, z);
		dict_write_tuplet(iter, &zval);

		app_message_outbox_send();		
	}

	set_timer();
}

static void set_timer(){
	if(current_state != IDLE){
		timer = app_timer_register(frequency, timer_handler, NULL);
	}
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
//  text_layer_set_text(text_layer, "Select");
	current_state = READY;
	set_timer();
	display_state();	
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
//  text_layer_set_text(text_layer, "Up");
	current_state = IDLE;
	set_timer();
	display_state();
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
//  text_layer_set_text(text_layer, "Down");
//	current_state = LOGGING;
	set_timer();
	display_state();
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  text_layer = text_layer_create((GRect) { .origin = { 0, 52 }, .size = { bounds.size.w, 80 } });
  text_layer_set_background_color(text_layer, GColorClear);
  text_layer_set_text_color(text_layer, GColorBlack);
  text_layer_set_text(text_layer, "Press a button");
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
  layer_add_child(window_layer, text_layer_get_layer(text_layer));
}

static void window_unload(Window *window) {
  text_layer_destroy(text_layer);
}

static void app_message_init()
{
	app_message_register_inbox_received(in_received_handler);
	app_message_register_inbox_dropped(in_dropped_handler);
	app_message_register_outbox_sent(out_sent_handler);
	app_message_register_outbox_failed(out_failed_handler);
	app_message_open(124, 124);
}

static void init(void) {
  window = window_create();
  window_set_background_color(window, GColorWhite);
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  const bool animated = true;
  window_stack_push(window, animated);

  accel_data_service_subscribe(0, NULL);

  current_state = IDLE;
  display_state();
  set_timer();
}

static void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();
  app_message_init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}
