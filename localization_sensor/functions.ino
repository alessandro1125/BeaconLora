void distanceScanCompletedCallback(MacAddress senderAddress,
                                   ibeacon_instance_t beacon) {
  Serial.println("Scansione ricevuta");
  auto got = scansMap->find(senderAddress.value);
  if (got == scansMap->end()) {
    Serial.println("Adding device to collection");
    ScansCollection toInsert = ScansCollection();
    toInsert.push_back({beacon, getCurrentTime()});
    scansMap->insert({senderAddress.value, toInsert});
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
    Serial.println("Distanza aggiunta");
  } else
    Serial.println("Distanza costante");
}

void sendCollectionToServer() {
  Serial.println("Inside sendcollection");
  Serial.print("Total size of map in bytes: ");
  unsigned long size = 0;
  for (auto i = scansMap->begin(); i != scansMap->end(); ++i) {
    size += i->second.capacity() * sizeof(scan_result) + sizeof(i->first);
  }
  Serial.println(size);

  Serial.print("Total size of old map in bytes: ");
  size = 0;
  for (auto i = oldMap->begin(); i != oldMap->end(); ++i) {
    size += i->second.capacity() * sizeof(scan_result);
  }
  Serial.println(size);

  if (!ibeacon_scanned_list
           .empty()) {  // cancello dalla lista di scansioni i beacon che non
                        // sono più visibili dopo un timeout
    for (int i = ibeacon_scanned_list.size() - 1; i >= 0; i--) {
      Serial.println("visiting ");
      Serial.println(ibeacon_scanned_list.at(i).minor);
      if (getCurrentTime() - ibeacon_scanned_list.at(i).lastTimestamp >=
          BEACON_TIMEOUT_SECONDS) {
        Serial.print("erasing ");
        Serial.println(ibeacon_scanned_list.at(i).minor);
        ibeacon_scanned_list.erase(ibeacon_scanned_list.begin() + i);
      }
    }
  }

  Serial.println("Size of ibeacon scanned list in bytes: ");
  Serial.println(ibeacon_scanned_list.capacity() * sizeof(ibeacon_instance_t));

  String JSON = "{" + JSON += "{\"eventsLog\":[";
  auto it = scansMap->begin();
  while (it != scansMap->end()) {
    uint64_t senderAddress = it->first;
    Serial.println(MacAddress(senderAddress).toString());
    ScansCollection* scans = &(it->second);
    for (size_t i = 0; i < (*scans).size(); i++) {
      char uuid[32];
      char* json;
      unsigned long time = getCurrentTime();
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
      char* temp = MacAddress(senderAddress).toCharArray();
      size_t needed = snprintf(
          NULL, 0,
          "{\"uuidBeacon\":\"%s\",\"majorVersion\":%d,\"minorVersion\":%d,"
          "\"idDevice\":\"%s\",\"timePosition\":%ld,\"latitudeDevice\":0."
          "000000,\"longitudeDevice\":0.000000,\"distanceBeacon\":%.2f}",
          uuid, (*scans)[i].beacon.major, (*scans)[i].beacon.minor, temp, time,
          (*scans)[i].beacon.distance);
      json = new char[needed + 1];
      sprintf(json,
              "{\"uuidBeacon\":\"%s\",\"majorVersion\":%d,\"minorVersion\":%d,"
              "\"idDevice\":\"%s\",\"timePosition\":%ld,\"latitudeDevice\":0."
              "000000,\"longitudeDevice\":0.000000,\"distanceBeacon\":%.2f}",
              uuid, (*scans)[i].beacon.major, (*scans)[i].beacon.minor, temp,
              time, (*scans)[i].beacon.distance);

      JSON += charArrayToString(json);
      JSON += ",";
      delete[] json;
      delete[] temp;
    }
    it++;
  }
  JSON += "{}]}";

  DevMap* tmp;
  tmp = scansMap;  // metto la mappa in un'altra vuota e la azzero, in modo da
                   // avere sia le letture che ho appena inviato che una mappa
                   // vuota in cui salvare i dati che ricevo nel frattempo che
                   // finisca la richiesta http
  scansMap = oldMap;
  oldMap = tmp;

  // Serial.println(JSON);
  requestUpdate.body = JSON;
  getHttpResponse(&requestUpdate, callBack_response, &display);
}

void callBack_response(String response) {
  requestUpdate.body = "";
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(response);
  if (!root.success()) {
    Serial.println("parseJsonObject failed");
    // oldMap->swap(scanMap);
    addOldMapToScansMap();  // se non sono riuscito a mettere i dati sul server
                            // li reinserisco nella mappa
  } else {
    const char* resultBuffer = root["MessageText"];
    String result = String(resultBuffer);
    if (result == "Action Completed") {
      Serial.println("Action completed");
      eraseMap(oldMap);
    } else {
      Serial.println("Malformad JSON");
      addOldMapToScansMap();
    }
  }
}

void addOldMapToScansMap() {
  if (oldMap == NULL) return;

  auto it = oldMap->begin();
  while (it != oldMap->end()) {
    MacAddress devMac = MacAddress(it->first);
    auto vec_it = it->second.begin();
    while (vec_it != it->second.end()) {
      distanceScanCompletedCallback(devMac, (*vec_it).beacon);
      vec_it++;
    }
    it++;
  }
  eraseMap(oldMap);
}

void eraseMap(DevMap* myMap) {
  DevMap temp;
  myMap->swap(temp);
}
