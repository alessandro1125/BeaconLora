#ifndef TIME_SYNC_H
#define TIME_SYNC_H

#include "http_helper.h"

extern unsigned long seconds_since_epoch;

unsigned long get_server_time_from_json(String json_string);
void time_sync_callback(String response);
void initTimeSync(U8X8_SSD1306_128X64_NONAME_SW_I2C * oled_display);
unsigned long getCurrentTime();

#endif

