#include <pebble.h>
#include "main.h"

static Window *s_window;

static TextLayer *s_status_text_layer;
static TextLayer *s_booking_layer;
static BitmapLayer *s_status_icon_layer;
static GBitmap *s_icon_bitmap = NULL;

static AppSync s_sync;
static uint8_t s_sync_buffer[64];
static bool Booked = false;

enum SmartoKey {
  SMARTO_STATUS_ICON_KEY = 0x0,      
  SMARTO_STATUS_TEXT_KEY = 0x1,
  SMARTO_BOOKING_KEY = 0x2,        
  SMARTO_JS_READY = 0x3,
};

static const uint32_t SMARTO_ICONS[] = {
  RESOURCE_ID_IMAGE_OCCUPIED,
  RESOURCE_ID_IMAGE_AVAILABLE,
  RESOURCE_ID_IMAGE_MAYBE,
  RESOURCE_ID_IMAGE_FETCH,
  RESOURCE_ID_IMAGE_ERROR
};

char *translate_error(AppMessageResult result) {
  switch (result) {
    case APP_MSG_OK: return "APP_MSG_OK";
    case APP_MSG_SEND_TIMEOUT: return "APP_MSG_SEND_TIMEOUT";
    case APP_MSG_SEND_REJECTED: return "APP_MSG_SEND_REJECTED";
    case APP_MSG_NOT_CONNECTED: return "APP_MSG_NOT_CONNECTED";
    case APP_MSG_APP_NOT_RUNNING: return "APP_MSG_APP_NOT_RUNNING";
    case APP_MSG_INVALID_ARGS: return "APP_MSG_INVALID_ARGS";
    case APP_MSG_BUSY: return "APP_MSG_BUSY";
    case APP_MSG_BUFFER_OVERFLOW: return "APP_MSG_BUFFER_OVERFLOW";
    case APP_MSG_ALREADY_RELEASED: return "APP_MSG_ALREADY_RELEASED";
    case APP_MSG_CALLBACK_ALREADY_REGISTERED: return "APP_MSG_CALLBACK_ALREADY_REGISTERED";
    case APP_MSG_CALLBACK_NOT_REGISTERED: return "APP_MSG_CALLBACK_NOT_REGISTERED";
    case APP_MSG_OUT_OF_MEMORY: return "APP_MSG_OUT_OF_MEMORY";
    case APP_MSG_CLOSED: return "APP_MSG_CLOSED";
    case APP_MSG_INTERNAL_ERROR: return "APP_MSG_INTERNAL_ERROR";
    default: return "UNKNOWN ERROR";
  }
}

static void sync_error_callback(DictionaryResult dict_error, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "In dropped: %i - %s", reason, translate_error(reason));
}

static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "sync_tuple_changed_callback %d", (int)key);
  switch (key) {
    case SMARTO_STATUS_ICON_KEY:
      if (s_icon_bitmap) {
        gbitmap_destroy(s_icon_bitmap);
      }

      s_icon_bitmap = gbitmap_create_with_resource(SMARTO_ICONS[new_tuple->value->uint8]);
      bitmap_layer_set_compositing_mode(s_status_icon_layer, GCompOpSet);
      bitmap_layer_set_bitmap(s_status_icon_layer, s_icon_bitmap);
      break;

    case SMARTO_BOOKING_KEY:
      text_layer_set_text(s_booking_layer, new_tuple->value->cstring);
      if(strlen(new_tuple->value->cstring) != 0) {
        if(Booked == false) {
          Booked = true;
        }
      } else if(Booked == true){
        Booked = false;
        static const uint32_t const segments[] = { 200, 100, 400 };
        VibePattern pat = {
          .durations = segments,
          .num_segments = ARRAY_LENGTH(segments),
        };
        vibes_enqueue_custom_pattern(pat);
      }
      break;

    case SMARTO_STATUS_TEXT_KEY:
      text_layer_set_text(s_status_text_layer, new_tuple->value->cstring);
      break;
  }
}

// Window Load event
static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_status_text_layer = text_layer_create(GRect(0, 5, bounds.size.w, 28));
  text_layer_set_text_color(s_status_text_layer, GColorWhite);
  text_layer_set_background_color(s_status_text_layer, GColorClear);
  text_layer_set_font(s_status_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(s_status_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_status_text_layer));

  s_status_icon_layer = bitmap_layer_create(GRect(0, 45, bounds.size.w, 80));
  layer_add_child(window_layer, bitmap_layer_get_layer(s_status_icon_layer));

  s_booking_layer = text_layer_create(GRect(0, 137, bounds.size.w, 32));
  text_layer_set_text_color(s_booking_layer, GColorWhite);
  text_layer_set_background_color(s_booking_layer, GColorClear);
  text_layer_set_font(s_booking_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_booking_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_booking_layer));

  Tuplet initial_values[] = {
    TupletInteger(SMARTO_STATUS_ICON_KEY, (uint8_t) 3),
    TupletCString(SMARTO_STATUS_TEXT_KEY, ""),
    TupletCString(SMARTO_BOOKING_KEY, "Fetching data..."),
    TupletInteger(SMARTO_JS_READY, (uint8_t) 0)
  };

  app_sync_init(&s_sync, s_sync_buffer, sizeof(s_sync_buffer),
      initial_values, ARRAY_LENGTH(initial_values),
      sync_tuple_changed_callback, sync_error_callback, NULL);
}

// Window Unload event
static void prv_window_unload(Window *window) {
  if (s_icon_bitmap) {
    gbitmap_destroy(s_icon_bitmap);
  }

  text_layer_destroy(s_booking_layer);
  text_layer_destroy(s_status_text_layer);
  bitmap_layer_destroy(s_status_icon_layer);
}

static void prv_init(void) {
  app_message_open(128, 128);

  s_window = window_create();
  window_set_background_color(s_window, PBL_IF_COLOR_ELSE(GColorBlack, GColorBlack));
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });

  window_stack_push(s_window, true);
}

static void prv_deinit(void) {
  if (s_window) {
    window_destroy(s_window);
  }
  app_sync_deinit(&s_sync);
}

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
}
