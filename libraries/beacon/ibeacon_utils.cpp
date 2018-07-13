#include "ibeacon_utils.h"

RSSI_params_t rssi_params;

esp_ble_scan_params_t ble_scan_params = {
    .scan_type              = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type          = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
    /*
        Scan interval. This is defined as the time interval from when the Controller started its last LE scan until it begins the subsequent LE scan. 
        Range: 0x0004 to 0x4000 Default: 0x0010 (10 ms) Time = N * 0.625 msec Time Range: 2.5 msec to 10.24 seconds 
    */
    .scan_interval          = 0x0100,
    /*
        Scan window. The duration of the LE scan. LE_Scan_Window shall be less than or equal to LE_Scan_Interval Range: 0x0004 to 0x4000 Default: 
        0x0010 (10 ms) Time = N * 0.625 msec Time Range: 2.5 msec to 10240 msec 
    */
    .scan_window            = 0x0100
};

void init_utils(){
	esp_ble_gap_set_scan_params(&ble_scan_params);
	init_RSSI_params();     
                     
}

int uuid_compare(uint8_t* uuid1, uint8_t* uuid2, size_t size){

    for(size_t i = 0; i < size; i++){
        if(uuid1[i] != uuid2[i]){
            return 0;
        }
    }
    return 1;
}

float avg_rssi(int8_t* misurations, size_t size){

	if(size == 1)
		return misurations[0];
    int mode_index = 0;
    int second_mode_index = 0;
    int frequency_count = 0;
    int second_frequency_count = 0;

    for(size_t a = 0; a < size; a++){
        int current_frequency_count = 0;
        for(size_t b = 0; b < size; b++){
            if(misurations[a] == misurations[b])
                current_frequency_count++;
        }
        if(current_frequency_count > frequency_count){
            frequency_count = current_frequency_count;
            mode_index = a;
        } else if (current_frequency_count > second_frequency_count){
            second_frequency_count = current_frequency_count;
            second_mode_index = a;
        }
    }

    if(frequency_count == second_frequency_count){
        return (misurations[mode_index]+ misurations[second_mode_index])/2;
    }

    return misurations[mode_index];
}


void distance_calculator(float rssiMeasured, float *distance){
    *distance = -1;
    const float ratio = rssiMeasured*1.0/rssi_params.reference_distance_RSSI;
    if (ratio < 1.0) {
        *distance = pow(ratio,10);
        Serial.printf("Device at %f m\n", *distance);
        return;
    }
	float esponente1 = (rssi_params.reference_distance_RSSI-rssiMeasured)/(10*rssi_params.lower_noise_boundary);
	float esponente2 = (rssi_params.reference_distance_RSSI-rssiMeasured)/(10*rssi_params.upper_noise_boundary);
	float distance1 = pow(10,esponente1); 
	float distance2 = pow(10,esponente2);
	Serial.printf("Device between %f m and %f m\n", distance2, distance1);
	*distance = (distance1+distance2)/2;
	return;
}

bool is_same_beacon(esp_ble_ibeacon_t* received_beacon, ibeacon_instance_t* registered_beacon){
	return uuid_compare(received_beacon->ibeacon_vendor.proximity_uuid, registered_beacon->proximity_uuid, (size_t)16) == 1
				&& ENDIAN_CHANGE_U16(received_beacon->ibeacon_vendor.major) == registered_beacon->major
				&& ENDIAN_CHANGE_U16(received_beacon->ibeacon_vendor.minor) == registered_beacon->minor;
}

void init_RSSI_params() {
		rssi_params.reference_distance_RSSI = -53;  //-70 per -4
		rssi_params.lower_noise_boundary = 3.2 - 0.15;
		rssi_params.upper_noise_boundary = 3.2 + 0.15;
}

void send_distance_to_server(ibeacon_instance_t* beacon){
    /*char* json = generate_json_data(beacon);
	Serial.printf("%s \n",json);
    request_instance_t request_instance;
    request_instance.web_host = "messages.geisoft.org";
    request_instance.web_port = 80;
    request_instance.web_path = "/services/beacontrace/feedposition";
    request_instance.method = "POST";
    request_instance.body = json;
    get_response(&request_instance, &distance_sent_callback);*/
}

void distance_sent_callback(char* response){
    //free(json_out_string);
    return;
}