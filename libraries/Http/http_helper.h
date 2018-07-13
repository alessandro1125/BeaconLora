#ifndef HTTP_HELPER_H
#define HTTP_HELPER_H

#include <WiFi.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <U8x8lib.h>
#include "../init_configs/init_configs_tools.h"


typedef void(*httpCallback) (String callback);

typedef struct{
    String web_host;
    String web_path;
    String body;
    String method;
}request_instance_t;

void getHttpResponse(request_instance_t * body, httpCallback callback, U8X8_SSD1306_128X64_NONAME_SW_I2C * oled_display);


#endif
