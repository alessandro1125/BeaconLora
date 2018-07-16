typedef struct scan_result {
  ibeacon_instance_t beacon;
  unsigned long timestamp;
};

typedef std::vector<scan_result> ScansCollection;
std::unordered_map<uint64_t, ScansCollection> scansMap;


void distanceScanCompletedCallback(MacAddress senderAddress, ibeacon_instance_t beacon){
  Serial.println("Scansion ricevuta");
  auto got = scansMap.find(senderAddress.value);
  if(got == scansMap.end()){
    Serial.println("Adding device to collection");
    ScansCollection toInsert = ScansCollection();
    toInsert.push_back({beacon, getCurrentTime()});
    scansMap.insert({senderAddress.value, toInsert});
    return;
  }
  ScansCollection* vec = &got->second;
  if((*vec).empty()){
    (*vec).push_back({beacon, getCurrentTime()});
    return;
  }
  float lastDist = (*((*vec).end() -1)).beacon.distance;
  if(beacon.distance < lastDist - DIST_ACCEPTANCE_INTERVAL || beacon.distance > lastDist + DIST_ACCEPTANCE_INTERVAL){
    (*vec).push_back({beacon, getCurrentTime()});
    Serial.println("Distanza aggiunta");
  }else
    Serial.println("Distanza costante");
}

void sendCollectionToServer(){
  for( auto b = ibeacon_scanned_list.begin(); b != ibeacon_scanned_list.end(); ){
      if( (*b).lastTimestamp - getCurrentTime() >= BEACON_TIMEOUT_SECONDS ) 
        b = ibeacon_scanned_list.erase(b); 
      else ++b;
  }
  /*String JSON = "{ \"idCustomer\": \""+ charArrayToString(current_configs.customer) +"\", \n";
  JSON += "\"idGroup\": \"" + charArrayToString(current_configs.collection) + "\", \n";
  JSON += "\"eventsLog\": [\n";
  auto it = scansMap.begin();
  while(it != scansMap.end()){
    uint64_t senderAddress = it-> first;
    ScansCollection* scans = &(it->second);
    for(size_t i = 0; i<(*scans).size(); i++){
      JSON += "{ \"idDevice\":\"" + MacAddress(senderAddress).toString() + "\",";
      JSON += "\"feedTime\":" + String((*scans)[i].timestamp) + ", \n";
      JSON += "\"TemperatureLog\":" + String((*scans)[i].temperature) + " \n";
      JSON += "}";
      if(i != (*scans).size() -1)
        JSON += ",\n";
    }
    it++;
  }
  JSON += "\n]}";
  Serial.println(JSON);
  requestUpdate.body = JSON;
  //Serial.println("Updating db");
  getHttpResponse(&requestUpdate, callBack_response, &u8x8);*/
}

void callBack_response(String response){
  //Controllo se solo se la risposta è corretta salvo la temperature
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(response);
  // Test if parsing succeeds.
  if (!root.success()) {
    Serial.println("parseJsonObject failed");
  }else{
    const char * resultBuffer = root["MessageText"];
    String result = String(resultBuffer);
    if(result == "Action Completed"){
      Serial.println("Action completed");
      auto it = scansMap.begin();
      while(it != scansMap.end()){
        it->second.erase(it->second.begin(), it->second.end());
        it++;
      }
      scansMap.erase(scansMap.begin(), scansMap.end());
      Serial.println("after sendCollection " + (String(esp_get_free_heap_size()) + " B"));
    }
  }
}


/*
*/
