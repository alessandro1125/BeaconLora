#ifndef IBEACON_UTILS_H
#define IBEACON_UTILS_H

#include <math.h>
#include <stdint.h>
#include "esp_ibeacon_api.h"
#include "esp_bt.h"
#include "esp_gattc_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "cJSON.h"

#include <sys/time.h>

#define pi 3.14159265358979323846

#define DEMO_TAG  "IBEACON_TRACKER"

typedef struct {

	float reference_distance_RSSI;         //Potenza alla distanza di riferimento (1m) = A nella formula
	float lower_noise_boundary;            //Limite inferiore del range per il rumore
	float upper_noise_boundary;            //limite superiore per il range del rumore

} RSSI_params_t;

extern RSSI_params_t rssi_params;

extern long seconds_since_epoch;

void init_utils();

int uuid_compare(uint8_t* uuid1, uint8_t* uuid2, size_t size);
float avg_rssi(int8_t* misurations, size_t size);
void distance_calculator(float rssiMeasured, float *distance);

bool is_same_beacon(esp_ble_ibeacon_t* received_beacon, ibeacon_instance_t* registered_beacon);

void init_RSSI_params();

void init_time_sync();
void time_sync_callback(char* response);

char* generate_json_data(ibeacon_instance_t* beacon);
long get_server_time_from_json(const char* const json_string);
void send_distance_to_server(ibeacon_instance_t* beacon);
void distance_sent_callback(char* response);

int compare_function(const void *a,const void *b);

#endif