#include "ibeacon_message_handler.h"

MacAddress address;
MacAddress node;

void ble_ibeacon_appRegister(){                                          
    esp_err_t status;
    //Serial.println(  "register callback");
    //register the scan callback function to the gap module
    if ((status = esp_ble_gap_register_callback(esp_gap_cb)) != ESP_OK) {
        //Serial.printf( "gap register error: %s\n", esp_err_to_name(status));
        return;
    }

	init_utils(); 
}

void handle_received_packet(esp_ble_gap_cb_param_t *scan_result, ibeacon_instance_t* ibeacon_scanned_list){
	
    if (scan_result->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_RES_EVT){
		/* Search for BLE iBeacon Packet */
		if (esp_ble_is_ibeacon_packet(scan_result->scan_rst.ble_adv, scan_result->scan_rst.adv_data_len)){
			esp_ble_ibeacon_t *ibeacon_data = (esp_ble_ibeacon_t*)(scan_result->scan_rst.ble_adv);
			handle_beacon_data(scan_result, ibeacon_data, ibeacon_scanned_list);
		}
    }
}

void handle_beacon_data(esp_ble_gap_cb_param_t *scan_result, esp_ble_ibeacon_t *ibeacon_data, ibeacon_instance_t* ibeacon_scanned_list) {
	//GESTIONE BEACONS
	if(!handle_beacon_messages(scan_result, ibeacon_data, ibeacon_scanned_list)){ //se il messaggio è di un beacon non ancora registrato
		register_new_beacon(scan_result, ibeacon_data, ibeacon_scanned_list);
	}

}

bool handle_beacon_messages(esp_ble_gap_cb_param_t *scan_result, esp_ble_ibeacon_t *ibeacon_data, ibeacon_instance_t* ibeacon_scanned_list){
	
	for(size_t i = 0; i < ibeacon_count; i++){

		if( is_same_beacon(ibeacon_data, &ibeacon_scanned_list[i]) ){

			//Controllo se la potenza di trasmissione è la stessa
			if(ibeacon_data->ibeacon_vendor.measured_power != ibeacon_scanned_list[i].measured_power){
				ibeacon_scanned_list[i].measured_power = ibeacon_data->ibeacon_vendor.measured_power;
			}

			//Se non sono alla scansione finale
			if(ibeacon_scanned_list[i].distance_scans_count < NUM_SCANNING_FOR_PRECISION ){
				//Update rssi measurement
				ibeacon_scanned_list[i].rssi_measured[ibeacon_scanned_list[i].distance_scans_count] = scan_result->scan_rst.rssi;
				ibeacon_scanned_list[i].distance_scans_count++;
				
			}else{
				//Scansioni completate, aggiorno la distanza del beacon e faccio ripartire il ciclo di scansioni
				//Calcolo la distanza
				ibeacon_scanned_list[i].avg_rssi = avg_rssi(ibeacon_scanned_list[i].rssi_measured, NUM_SCANNING_FOR_PRECISION);
				distance_calculator(ibeacon_scanned_list[i].avg_rssi, &ibeacon_scanned_list[i].distance);

				//if(completed_reference_devices >= total_reference_devices && total_reference_devices != 0)
				
				Packet pack = locationScanPacket(address, node, ibeacon_scanned_list[i].distance);
				pack.printPacket();
				int res = sendPacket(pack);
				Helpers::printResponseMessage(res);
				
				ibeacon_scanned_list[i].distance_scans_count = 0;
			}
			return true;
		}
	}
	return false;
}

void register_new_beacon(esp_ble_gap_cb_param_t *scan_result, esp_ble_ibeacon_t *ibeacon_data, ibeacon_instance_t* ibeacon_scanned_list) {
	//Create new beacon
	ibeacon_count++;
	for (uint8_t i = 0; i < 16; i++) {
		ibeacon_scanned_list[ibeacon_count-1].proximity_uuid[i] = ibeacon_data->ibeacon_vendor.proximity_uuid[i];
	}
	ibeacon_scanned_list[ibeacon_count-1].major = ENDIAN_CHANGE_U16(ibeacon_data->ibeacon_vendor.major);
	ibeacon_scanned_list[ibeacon_count-1].minor = ENDIAN_CHANGE_U16(ibeacon_data->ibeacon_vendor.minor);
	ibeacon_scanned_list[ibeacon_count-1].measured_power = ibeacon_data->ibeacon_vendor.measured_power;
	ibeacon_scanned_list[ibeacon_count-1].distance = -1;
	ibeacon_scanned_list[ibeacon_count-1].distance_scans_count = 0;
	//Serial. println(  "NEW BEACON REGISTERED");
	//Serial. printf(  "%d %d \n", ibeacon_scanned_list[ibeacon_count-1].major, ibeacon_scanned_list[ibeacon_count-1].minor);	
}

Packet locationScanPacket(MacAddress sender, MacAddress dest, float distance){
  char body[LOCATION_SCAN_PACKET_LENGTH];
  int16_t distInt = (int16_t) distance;
  body[0] = ((distInt & 0xFF00) >> 8);
  body[1] = ((distInt & 0x00FF) >> 0);
  distance -= distInt;
  distance *= 100;
  int8_t distFloat = (int8_t) distance;
  body[2] = distFloat;
  return Packet(dest,sender,PACKET_TYPE_NORM | PACKET_TYPE_LOCATION_SCAN , packetCounter,body,LOCATION_SCAN_PACKET_LENGTH); 
}