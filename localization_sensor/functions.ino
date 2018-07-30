void distanceScanCompletedCallback(MacAddress senderAddress,
                                   ibeacon_instance_t beacon) {
  auto got = scansMap.find(senderAddress.value);
  if (got == scansMap.end()) {
    ScansCollection toInsert = ScansCollection();
    toInsert.push_back({beacon, getCurrentTime()});
    scansMap.insert({senderAddress.value, toInsert});
    return;
  }
  ScansCollection* vec = &got->second;
  if ((*vec).empty()) {
    (*vec).push_back({beacon, getCurrentTime()});
    return;
  }
  float lastDist = (*((*vec).end() - 1)).beacon.distance;
  if (beacon.distance < lastDist - DIST_ACCEPTANCE_INTERVAL ||
      beacon.distance > lastDist + DIST_ACCEPTANCE_INTERVAL) {
    (*vec).push_back({beacon, getCurrentTime()});
  }
}

void sendCollectionToServer() {
  Serial.println(F("Inside sendcollection"));
  requestUpdate.body = new String("{\"eventsLog\":[");
  auto it = scansMap.begin();
  while (it != scansMap.end()) {
    uint64_t senderAddress = it->first;
    Serial.println(MacAddress(senderAddress).toString());
    ScansCollection* scans = &(it->second);
    for (size_t i = 0; i < (*scans).size(); i++) {
      char uuid[36];
      unsigned long scan_time = (*scans)[i].timestamp;
      sprintf(uuid, "%x%x%x%x-%x%x-%x%x-%x%x%x%x%x%x%x%x",
              (unsigned char)(*scans)[i].beacon.proximity_uuid[0],
              (unsigned char)(*scans)[i].beacon.proximity_uuid[1],
              (unsigned char)(*scans)[i].beacon.proximity_uuid[2],
              (unsigned char)(*scans)[i].beacon.proximity_uuid[3],
              (unsigned char)(*scans)[i].beacon.proximity_uuid[4],
              (unsigned char)(*scans)[i].beacon.proximity_uuid[5],
              (unsigned char)(*scans)[i].beacon.proximity_uuid[6],
              (unsigned char)(*scans)[i].beacon.proximity_uuid[7],
              (unsigned char)(*scans)[i].beacon.proximity_uuid[8],
              (unsigned char)(*scans)[i].beacon.proximity_uuid[9],
              (unsigned char)(*scans)[i].beacon.proximity_uuid[10],
              (unsigned char)(*scans)[i].beacon.proximity_uuid[11],
              (unsigned char)(*scans)[i].beacon.proximity_uuid[12],
              (unsigned char)(*scans)[i].beacon.proximity_uuid[13],
              (unsigned char)(*scans)[i].beacon.proximity_uuid[14],
              (unsigned char)(*scans)[i].beacon.proximity_uuid[15]);
      *requestUpdate.body +=
          "{\"uuidBeacon\":\"" + wifi_config::charArrayToString(uuid) + "\",";
      *requestUpdate.body +=
          "\"majorVersion\":" + String((*scans)[i].beacon.major) + ",";
      *requestUpdate.body +=
          "\"minorVersion\":" + String((*scans)[i].beacon.minor) + ",";
      *requestUpdate.body +=
          "\"idDevice\":\"" + MacAddress(senderAddress).toString() + "\",";
      *requestUpdate.body += "\"timePosition\":" + String(scan_time) + ",";
      *requestUpdate.body +=
          "\"latitudeDevice\": 0.000000, \"longitudeDevice\": 0.000000,";
      *requestUpdate.body +=
          "\"distanceBeacon\":" + String((*scans)[i].beacon.distance);
      *requestUpdate.body += "},";
    }
    it++;
  }
  *requestUpdate.body += "{}]}";
  Serial.println(F("JSON generated"));
  scansMap.swap(oldMap);  // metto la mappa in un'altra vuota e la azzero, in
                          // modo da avere sia le letture che ho appena inviato
                          // che una mappa vuota in cui salvare i dati che
                          // ricevo nel frattempo che finisca la richiesta http

  Serial.println(F("Map swapped"));
  Serial.println(F("before Http sent"));
  getHttpResponse(&requestUpdate, callBack_response);
  Serial.println(F("Http sent"));
}

void callBack_response(String response) {
  delete (requestUpdate.body);
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(response);
  if (!root.success()) {
    Serial.println("parseJsonObject failed");
    // oldMap.swap(scanMap);
    addOldMapToScansMap();  // se non sono riuscito a mettere i dati sul server
                            // li reinserisco nella mappa
  } else {
    const char* resultBuffer = root["MessageText"];
    String result = String(resultBuffer);
    if (result == "Action Completed") {
      Serial.println("Action completed");
      eraseOldMap();
    } else {
      Serial.println("Malformed JSON");
      addOldMapToScansMap();
    }
  }
}

void addOldMapToScansMap() {
  auto it = oldMap.begin();
  while (it != oldMap.end()) {
    MacAddress devMac = MacAddress(it->first);
    auto vec_it = it->second.begin();
    while (vec_it != it->second.end()) {
      distanceScanCompletedCallback(devMac, (*vec_it).beacon);
      vec_it++;
    }
    it++;
  }
  eraseOldMap();
}

void eraseOldMap() {
  DevMap temp;
  oldMap.swap(temp);
}
