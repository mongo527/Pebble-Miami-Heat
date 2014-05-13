#include <pebble.h>

Window *window;
TextLayer *time_layer;
TextLayer *date_layer;
TextLayer *white_layer;
TextLayer *battery_layer;
TextLayer *game_layer;

Layer *status_layer;
Layer *root;

static GBitmap *background;
static BitmapLayer *bg_layer;

static GBitmap *bt;
static BitmapLayer *bt_layer;

time_t start;

enum {
    AKEY_GET_EVENTS = 0,
    AKEY_GAME_TIME = 1,
    AKEY_HOME_TEAM = 2,
    AKEY_AWAY_TEAM = 3,
    AKEY_HOME_SCORE = 4,
    AKEY_AWAY_SCORE = 5,
};

void get_events_handler(char* date) {
    Tuplet events_tuple = TupletCString(AKEY_GET_EVENTS, date);
	
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	
	if (iter == NULL) {
		return;
	}
	
	dict_write_tuplet(iter, &events_tuple);
	dict_write_end(iter);
	
	app_message_outbox_send();
}

static void battery_handler(BatteryChargeState battery) {
    static char batt[] = "100%";

    snprintf(batt, sizeof(batt), "%d%%", battery.charge_percent);
    text_layer_set_text(battery_layer, batt);
}

static void time_handler(struct tm* tick_time, TimeUnits units_changed) {

    static char time_text[] = "00:00"; // Needs to be static because it's used by the system later.
    static char date_text[] = "Xxxxxxxxxxx Xxxxxxxxxx 00";
    static char date_num[] = "00000000";
    
    char *time_format;
    
    strftime(date_text, sizeof(date_text), "%A, %b %e", tick_time);
    text_layer_set_text(date_layer, date_text);
    
    if(clock_is_24h_style()) {
        time_format = "%R";
    }
    else {
        time_format = "%I:%M";
    }
    
    strftime(time_text, sizeof(time_text), time_format, tick_time);

    // Kludge to handle lack of non-padded hour format string
    // for twelve hour clock.
    if (!clock_is_24h_style() && (time_text[0] == '0')) {
        memmove(time_text, &time_text[1], sizeof(time_text) - 1);
    }
    
    text_layer_set_text(time_layer, time_text);
    battery_handler(battery_state_service_peek());
    
    strftime(date_num, sizeof(date_num), "%Y%m%d", tick_time);
    
    if(time(NULL) - start >= 300 || time(NULL) - start == 0) {
        get_events_handler(date_num);
        time(&start);
    }
}

static void bluetooth_handler(bool connected) {
    if (connected == true) {
        bitmap_layer_set_bitmap(bt_layer, bt);
    }
    layer_mark_dirty(bitmap_layer_get_layer(bt_layer));
}

void tap_handler(AccelAxisType axis, int32_t direction){
    time_t now = time(NULL);
    
    if(layer_get_hidden(text_layer_get_layer(date_layer)) == 0) {
        layer_set_hidden(text_layer_get_layer(date_layer), true);
        layer_set_hidden(text_layer_get_layer(game_layer), false);
    }
    
    else if(layer_get_hidden(text_layer_get_layer(date_layer)) == 1) {
        layer_set_hidden(text_layer_get_layer(game_layer), true);
        layer_set_hidden(text_layer_get_layer(date_layer), false);
    }
    
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Tap Handler");
}

void handle_init(void) {
    window = window_create();
    window_stack_push(window, true);
    window_set_background_color(window, GColorBlack);
    
    root = window_get_root_layer(window);
    GRect frame = layer_get_frame(root);
    
    background = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_HEAT_WHITE);
    bg_layer = bitmap_layer_create(GRect(0, 1, frame.size.w, 100));
    bitmap_layer_set_background_color(bg_layer, GColorClear);
    bitmap_layer_set_bitmap(bg_layer, background);
    bitmap_layer_set_compositing_mode(bg_layer, GCompOpOr);
    
    date_layer = text_layer_create(GRect(0, 140, frame.size.w, 26));
    text_layer_set_text_color(date_layer, GColorBlack);
    text_layer_set_background_color(date_layer, GColorClear);
    text_layer_set_font(date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    text_layer_set_text_alignment(date_layer, GTextAlignmentCenter);
    
    white_layer = text_layer_create(GRect(0, 105, frame.size.w, 63));
    text_layer_set_background_color(white_layer, GColorWhite);
    
    time_layer = text_layer_create(GRect(0, 100, frame.size.w, 68));
    text_layer_set_text_color(time_layer, GColorBlack);
    text_layer_set_background_color(time_layer, GColorClear);
    text_layer_set_font(time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
    text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
    
    status_layer = layer_create(GRect(0, 0, frame.size.w, 20));
    
    battery_layer = text_layer_create(GRect(104, 0, /* width */ 40, 20 /* height */));
    text_layer_set_text_color(battery_layer, GColorWhite);
    text_layer_set_background_color(battery_layer, GColorClear);
    text_layer_set_font(battery_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    text_layer_set_text_alignment(battery_layer, GTextAlignmentRight);
    text_layer_set_text(battery_layer, "---");
    
    bt = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BT_WHITE);
    bt_layer = bitmap_layer_create(GRect(0, 0, 20, 20));
    bitmap_layer_set_background_color(bt_layer, GColorClear);
    bluetooth_handler(bluetooth_connection_service_peek());
    //bitmap_layer_set_compositing_mode(bt_layer, GCompOpOr);
    
    layer_add_child(status_layer, text_layer_get_layer(battery_layer));
    layer_add_child(status_layer, bitmap_layer_get_layer(bt_layer));
    
    game_layer = text_layer_create(GRect(0, 140, /* width */ frame.size.w, 26 /* height */));
    //game_layer = text_layer_create(GRect(0, 5, /* width */ frame.size.w, 100 /* height */));
    text_layer_set_text_color(game_layer, GColorBlack);
    text_layer_set_background_color(game_layer, GColorClear);
    text_layer_set_font(game_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    text_layer_set_text_alignment(game_layer, GTextAlignmentCenter);
    
    tick_timer_service_subscribe(MINUTE_UNIT, &time_handler);
    battery_state_service_subscribe(&battery_handler);
    bluetooth_connection_service_subscribe(&bluetooth_handler);
    accel_tap_service_subscribe(tap_handler);
    
    layer_add_child(root, text_layer_get_layer(white_layer));
    layer_add_child(root, text_layer_get_layer(time_layer));
    layer_add_child(root, text_layer_get_layer(date_layer));
    layer_add_child(root, text_layer_get_layer(game_layer));
    layer_set_hidden(text_layer_get_layer(game_layer), true);
    layer_add_child(root, bitmap_layer_get_layer(bg_layer));
    layer_add_child(root, status_layer);
}

void handle_deinit(void) {
    tick_timer_service_unsubscribe();
    battery_state_service_unsubscribe();
    bluetooth_connection_service_unsubscribe();
    accel_tap_service_unsubscribe();
    
    bitmap_layer_destroy(bt_layer);
    gbitmap_destroy(bt);
    
    bitmap_layer_destroy(bg_layer);
    gbitmap_destroy(background);
    
    text_layer_destroy(time_layer);
    text_layer_destroy(date_layer);
    text_layer_destroy(battery_layer);
    text_layer_destroy(game_layer);
}

void events_in_received_handler(DictionaryIterator *iter) {
    Tuple *gameTime = dict_find(iter, AKEY_GAME_TIME);
    Tuple *homeTeam = dict_find(iter, AKEY_HOME_TEAM);
    Tuple *awayTeam = dict_find(iter, AKEY_AWAY_TEAM);
        
    static char game[] = "00:00 - AAA @ BBB";
    
    if(strcmp(gameTime->value->cstring, "No Game Today") == 0) {
        snprintf(game, sizeof(game), "No Game Today");
    }
    else {
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "%s - %s @ %s", gameTime->value->cstring, awayTeam->value->cstring, homeTeam->value->cstring);
        snprintf(game, sizeof(game), "%s - %s @ %s", gameTime->value->cstring, awayTeam->value->cstring, homeTeam->value->cstring);
    }
        
    text_layer_set_text(game_layer, game);
    layer_mark_dirty(text_layer_get_layer(game_layer));
}

void scores_in_received_handler(DictionaryIterator *iter) {
    Tuple *homeTeam = dict_find(iter, AKEY_HOME_TEAM);
    Tuple *awayTeam = dict_find(iter, AKEY_AWAY_TEAM);
    Tuple *homeScore = dict_find(iter, AKEY_HOME_SCORE);
    Tuple *awayScore = dict_find(iter, AKEY_AWAY_SCORE);
    
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "%s: %d - %s: %d", awayTeam->value->cstring, awayScore->value->int16, homeTeam->value->cstring, homeScore->value->int16);
    
    static char score[] = "AAA: ### - BBB: ###";
    snprintf(score, sizeof(score), "%s: %d - %s: %d", awayTeam->value->cstring, awayScore->value->int16, homeTeam->value->cstring, homeScore->value->int16);
    
    text_layer_set_text(game_layer, score);
    layer_mark_dirty(text_layer_get_layer(game_layer));
}

static void in_received_handler(DictionaryIterator *iter, void *context) {
    Tuple *gameTime_tuple = dict_find(iter, AKEY_GAME_TIME);
    Tuple *homeScore_tuple = dict_find(iter, AKEY_HOME_SCORE);
    
    if(gameTime_tuple) {
        events_in_received_handler(iter);
    }
    
    else if(homeScore_tuple) {
        scores_in_received_handler(iter);
    }
}

static void app_message_init(void) {
	app_message_register_inbox_received(in_received_handler);
	//app_message_register_inbox_dropped(in_dropped_handler);
	//app_message_register_outbox_failed(out_failed_handler);
	
	app_message_open(256, 256);
}

int main(void) {
    handle_init();
    app_message_init();
    
    //get_events_handler();
    //time(&start);
    
    app_event_loop();
    handle_deinit();
}
