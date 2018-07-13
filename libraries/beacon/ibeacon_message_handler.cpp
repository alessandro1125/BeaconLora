#include "ibeacon_message_handler.h"

void ble_ibeacon_appRegister(){                                          
    esp_err_t status;
    if ((status = esp_ble_gap_register_callback(esp_gap_cb)) != ESP_OK) {
        return;
    }

	init_utils(); 
}

void handle_received_packet(esp_ble_gap_cb_param_t *scan_result, ibeacon_instance_t* ibeacon_scanned_list){
	
    if (scan_result->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_RES_EVT){
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
				//myaddress decalred in loraprotocol.h
				Packet pack = locationScanPacket(myAddress, nodeAddress, ibeacon_scanned_list[i].distance);
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

void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    esp_err_t err;
    switch (event) {
        case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
            //the unit of the duration is second, 0 means scan permanently
            Serial.printf("scan param set complete\n");
            uint32_t duration = 0;
            esp_ble_gap_start_scanning(duration);
            break;
        }
        case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
            //scan start complete event to indicate scan start successfully or failed
            if ((err = param->scan_start_cmpl.status) != ESP_BT_STATUS_SUCCESS) {
                Serial.printf("Scan start failed: %s\n", esp_err_to_name(err));
            }
            break;
        case ESP_GAP_BLE_SCAN_RESULT_EVT: {
            esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;
      //gestisce il pacchetto in arrivo
            handle_received_packet( scan_result, ibeacon_scanned_list );
            break;
        }

        case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
            if ((err = param->scan_stop_cmpl.status) != ESP_BT_STATUS_SUCCESS){
                Serial.printf("Scan stop failed: %s\n", esp_err_to_name(err));
            }
            else {
                Serial.println("Stop scan successfully");
            }
            break;
        default:
            break;
    }
}