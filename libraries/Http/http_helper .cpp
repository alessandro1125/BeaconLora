#include "http_helper.h"

const char* ssid_id     = "GEISOFT-GUEST";
const char* password_id = "GEISOFTGUEST01";

void (*response_callback)(String response);


void getHttpResponse(request_instance_t * instance_request, httpCallback callback, U8X8_SSD1306_128X64_NONAME_SW_I2C * oled_display){

  //Serial.println("Updating db");
  //Serial.print("Sending httpRequest to: ");
  Serial.println((* instance_request).web_host);

  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect((* instance_request).web_host.c_str(), httpPort)) {
      Serial.println("connection failed");
      if(WiFi.status() != WL_CONNECTED){
          connectToWifi(&current_configs.wifi_configs,oled_display);
      }
      return;
  }

  /*
  // We now create a URI for the request
  String url = "/services/beacontrace/feedtemp";
  */

  //Serial.print("Requesting URL: ");
  //Serial.println((* instance_request).web_path);

  // This will send the request to the server
  String request = (* instance_request).method + " " + (* instance_request).web_path + " HTTP/1.1\r\n" +
                "Host: " + (* instance_request).web_host + "\r\n" +
                "content-type: application/json\r\n" +
                "authorization: Basic dGVsZWNvOnRtYXRlMjA=\r\n" +
                "content-length: " + (* instance_request).body.length() + "\r\n" +
                "\r\n"+
                (* instance_request).body;
  //Serial.println(request);
  client.print(request);
  unsigned long timeout = millis();
  while (client.available() == 0) {
      if (millis() - timeout > 5000) {
          Serial.println(">>> Client Timeout !");
          client.stop();
          return;
      }
  }

  String response = "";
  // Read all the lines of the reply from server and print them to Serial
  while(client.available()) {
      String line = client.readStringUntil('\r');
      response += line;
  }
  char* responseArray = &response[0];
  String bodyResponse = "";
  for (int i = 0; i < response.length(); i++){
      if(responseArray[0] != '{')
        responseArray++;
      else
        break;
  }

  bodyResponse = responseArray;
  //Serial.println(bodyResponse);
  
  
  callback(bodyResponse);
  Serial.println();
  Serial.println("closing connection");
}

