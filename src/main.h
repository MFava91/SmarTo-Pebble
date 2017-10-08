#pragma once
#include <pebble.h>

static void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context);
static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context);
static void prv_inbox_received_handler(DictionaryIterator *iter, void *context);
static void prv_window_load(Window *window);
static void prv_window_unload(Window *window);
static void prv_init(void);
static void prv_deinit(void);
