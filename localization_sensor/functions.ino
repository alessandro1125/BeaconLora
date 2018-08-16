void distanceScanCompletedCallback(MacAddress senderAddress,
                                   BeaconInfo beacon) {
  while (
      !mutex.try_lock()) {  // aspetto fino a quando ho l'accesso esclusivo
                            // alla risorsa: questa è una funzione che viene
                            // utilizzata sia dal thread del ble che dal thread
                            // http e dal thread principale. Io voglio che solo
                            // uno dei tre la stia eseguendo in contemporanea
    delay(10);
  }
  Serial.print(F("Mutex locked by: "));
  Serial.println(pcTaskGetTaskName(NULL));
  auto got = scansMap.find(senderAddress.value);

  if (got == scansMap.end()) {
    ScansCollection toInsert = ScansCollection();
    toInsert.push_back(beacon);
    scansMap.insert({senderAddress.value, toInsert});
    Serial.println(F("Mutex unlocked"));
    mutex.unlock();  // libero la risorsa
    return;
  }

  ScansCollection* vec = &got->second;
  if ((*vec).empty()) {
    (*vec).push_back(beacon);
    Serial.println(F("Mutex unlocked"));
    mutex.unlock();  // libero la risorsa
    return;
  }

  float lastDist = (*((*vec).end() - 1)).beacon.distance;
  if (beacon.beacon.distance < lastDist - DIST_ACCEPTANCE_INTERVAL ||
      beacon.beacon.distance > lastDist + DIST_ACCEPTANCE_INTERVAL) {
    (*vec).push_back(beacon);
  }
  Serial.println(F("Mutex unlocked"));
  mutex.unlock();  // libero la risorsa
}

void sendCollectionToServer() {
  while (
      !mutex.try_lock()) {  // aspetto il momento in cui nessuno sta accedendo
                            // alla risorsa scansMap per poterci accedere
    delay(10);
  }
  Serial.print(F("Mutex locked by: "));
  Serial.println(pcTaskGetTaskName(NULL));

  scansMap.swap(oldMap);  // metto la mappa in un'altra vuota e la azzero, in
                          // modo da avere sia le letture che ho appena inviato
                          // che una mappa vuota in cui salvare i dati che
                          // ricevo nel frattempo che finisca la richiesta http

  // Da qui in poi non uso più scansMap quindi posso liberare il mutex

  Serial.println(F("Mutex unlocked"));
  mutex.unlock();
  requestUpdate.body = new String("{\"idDevice\": \"" + myAddress.toString() +
                                  "\", \"eventsLog\":[");
  auto it = oldMap.begin();
  while (it != oldMap.end()) {
    uint64_t senderAddress = it->first;
    Serial.println(MacAddress(senderAddress).toString());
    ScansCollection* scans = &(it->second);
    for (size_t i = 0; i < (*scans).size(); i++) {
      char uuid[36];
      unsigned long scan_time = (*scans)[i].beacon.lastTimestamp;
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
      *requestUpdate.body += "{\"uuidBeacon\":\"" + String(uuid) + "\",";
      *requestUpdate.body +=
          "\"majorVersion\":" + String((*scans)[i].beacon.major) + ",";
      *requestUpdate.body +=
          "\"minorVersion\":" + String((*scans)[i].beacon.minor) + ",";
      *requestUpdate.body +=
          "\"idDevice\":\"" + MacAddress(senderAddress).toString() + "\",";
      *requestUpdate.body += "\"timePosition\":" + String(scan_time) + ",";
      *requestUpdate.body += "\"latitudeDevice\": " + String((*scans)[i].x) +
                             ", \"longitudeDevice\": " + String((*scans)[i].y) +
                             ",";
      *requestUpdate.body +=
          "\"distanceBeacon\":" + String((*scans)[i].beacon.distance);
      *requestUpdate.body += "},";
    }
    it++;
  }
  *requestUpdate.body += "{}]}";
  // Serial.println(*requestUpdate.body);
  Serial.println(F("About to call gethttpresponse"));
  getHttpResponse(&requestUpdate, callBack_response);
}

// ATTENZIONE: questa funzione viene eseguita all'interno del task http anche se
// è dichiarata qui, anche addOldMapToScansMap

void callBack_response(String response) {
  delete (requestUpdate.body);

  if (response == "timeout") {
    display.setRow(2, "Conn. timeout");
    checkEccessiveTimeouts();
    return;
  }

  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(response);

  if (!root.success()) {
    Serial.println("parseJsonObject failed");
    display.setRow(2, "Conn. fallita");
    checkEccessiveTimeouts();

  } else {
    const char* resultBuffer = root["MessageText"];
    String result = String(resultBuffer);

    if (result == "Action Completed") {
      Serial.println("Action completed");
      display.setRow(2, "Op. completata");
      eraseOldMap();
      timeoutCount = 0;

    } else {
      display.setRow(2, "Conn. fallita");
      Serial.println("Malformed JSON");
      checkEccessiveTimeouts();
    }
  }
}

void checkEccessiveTimeouts() {
  timeoutCount++;
  Serial.print(F("TimeoutCount: "));
  Serial.println(timeoutCount);
  if (timeoutCount > 2) {
    eraseOldMap();
    timeoutCount = 0;
  } else
    addOldMapToScansMap();  // se non sono riuscito a mettere i dati sul
                            // server li reinserisco nella mappa
}

void addOldMapToScansMap() {
  auto it = oldMap.begin();
  while (it != oldMap.end()) {
    MacAddress devMac = MacAddress(it->first);
    auto vec_it = it->second.begin();
    while (vec_it != it->second.end()) {
      distanceScanCompletedCallback(devMac, (*vec_it));
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
