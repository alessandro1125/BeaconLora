#ifndef HTTP_HELPER_H
#define HTTP_HELPER_H

#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <WiFi.h>

#define EXAMPLE_WIFI_SSID "GEISOFT-GUEST"
#define EXAMPLE_WIFI_PASS "GEISOFTGUEST01"


/* Constants that aren't configurable in menuconfig */
#define WEB_SERVER "messages.geisoft.org"
#define WEB_PORT 80
#define WEB_URL "/services/beacontrace/feedposition"

typedef struct{
    char * web_host;
    int web_port;
    char * web_path;
    char * body;
    char * method;
}request_instance_t;

extern request_instance_t request_instance;

static const char *TAG = "example";

void get_response(request_instance_t * body, void (*callBack)(char * response));
extern void (*response_callback)(char * response);
void start_http_task();

#endif