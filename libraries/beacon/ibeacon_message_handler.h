#ifndef IBEACON_MESSAGE_HANDLER_H
#define IBEACON_MESSAGE_HANDLER_H

#include "../LoRa/LoRaProtocol.h"
#include "../init_configs/mac_address.h"

#include "ibeacon_utils.h"

#define ENDIAN_CHANGE_U16(x) ((((x)&0xFF00)>>8) + (((x)&0xFF)<<8))

#define LOCATION_SCAN_PACKET_LENGTH 3
#define PACKET_TYPE_LOCATION_SCAN 8

extern MacAddress address;
extern MacAddress node;

void ble_ibeacon_appRegister();
void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);

void handle_received_packet(esp_ble_gap_cb_param_t *scan_result, ibeacon_instance_t* ibeacon_scanned_list);
void handle_beacon_data(esp_ble_gap_cb_param_t *scan_result, esp_ble_ibeacon_t *ibeacon_data, ibeacon_instance_t* ibeacon_scanned_list);
void register_new_beacon(esp_ble_gap_cb_param_t *scan_result, esp_ble_ibeacon_t *ibeacon_data, ibeacon_instance_t* ibeacon_scanned_list);

//ritorna vero se ha trovato il beacon tra quelli registrati, falso altrimenti
bool handle_beacon_messages(esp_ble_gap_cb_param_t *scan_result, esp_ble_ibeacon_t *ibeacon_data, ibeacon_instance_t* ibeacon_scanned_list); 
Packet locationScanPacket(MacAddress sender, MacAddress dest, float distance);

#endif

