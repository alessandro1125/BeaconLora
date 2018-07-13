
#ifndef ESP_IBEACON_API_H
#define ESP_IBEACON_API_H

#include <stdint.h>
#include <cstring>
#include "esp_gap_ble_api.h"

//Max num of scanning beacon
#define MAX_SCANNING_BEACONS 20
#define NUM_SCANNING_FOR_PRECISION 5

/* Major and Minor part are stored in big endian mode in iBeacon packet,
 * need to use this macro to transfer while creating or processing
 * iBeacon data */
#define ENDIAN_CHANGE_U16(x) ((((x)&0xFF00)>>8) + (((x)&0xFF)<<8))

typedef struct {
    uint8_t flags[3];
    uint8_t length;
    uint8_t type;
    uint16_t company_id;
    uint16_t beacon_type;
}__attribute__((packed)) esp_ble_ibeacon_head_t;

typedef struct {
    uint8_t proximity_uuid[16];
    uint16_t major;
    uint16_t minor;
    int8_t measured_power;
}__attribute__((packed)) esp_ble_ibeacon_vendor_t;

typedef struct {
    esp_ble_ibeacon_head_t ibeacon_head;
    esp_ble_ibeacon_vendor_t ibeacon_vendor;
}__attribute__((packed)) esp_ble_ibeacon_t;

typedef struct {
    uint8_t proximity_uuid[16];
    uint16_t major;
    uint16_t minor;
    int8_t measured_power;
    int8_t rssi_measured[NUM_SCANNING_FOR_PRECISION];
    uint16_t distance_scans_count;//Initialize at zero
    float avg_rssi;
    float distance;
}__attribute__((packed)) ibeacon_instance_t;

//The list of scanned beacons
extern ibeacon_instance_t ibeacon_scanned_list[MAX_SCANNING_BEACONS];
extern size_t ibeacon_count;

/* For iBeacon packet format, please refer to Apple "Proximity Beacon Specification" doc */
/* Constant part of iBeacon data */
extern esp_ble_ibeacon_head_t ibeacon_common_head;

bool esp_ble_is_ibeacon_packet (uint8_t *adv_data, uint8_t adv_data_len);

esp_err_t esp_ble_config_ibeacon_data (esp_ble_ibeacon_vendor_t *vendor_config, esp_ble_ibeacon_t *ibeacon_adv_data);

#endif