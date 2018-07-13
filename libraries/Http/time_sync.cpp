#include "time_sync.h"

unsigned long seconds_since_epoch;

unsigned long get_server_time_from_json(String json_string){
    StaticJsonBuffer<200> jsonBuffer;
	JsonObject& root = jsonBuffer.parseObject(json_string);

	if (!root.success()) {
    	Serial.println("parseObject() failed");
    	return -1;
  	}

    unsigned long serverTime = root["serverTime"];
    return serverTime;
}

void time_sync_callback(String response){
    seconds_since_epoch = get_server_time_from_json(response);
    printf("seconds since epoch %ld \n",seconds_since_epoch);
    time_t now;
    time(&now);
    seconds_since_epoch -= now;  //perchè dopo riaggiungo now nel conto
}

void initTimeSync(U8X8_SSD1306_128X64_NONAME_SW_I2C * oled_display){
	request_instance_t request_instance;
    request_instance.web_host = "messages.geisoft.org";
    request_instance.web_path = "/services/beacontrace/sincrotime";
    request_instance.method = "GET";
    request_instance.body = "";
    getHttpResponse(&request_instance, &time_sync_callback, oled_display);
}

unsigned long getCurrentTime(){
	time_t now;
    time(&now);
	return seconds_since_epoch + now;
}