#include "http_helper.h"

request_instance_t request_instance;
void (*response_callback)(char * response);

static void initialise_wifi(void)
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(EXAMPLE_WIFI_SSID, EXAMPLE_WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED)
        Serial.print(".");
    Serial.println("");
}

static void http_get_task(void *pvParameters)
{

    //Building request
    char request_buffer[512];
    

    //inizializzare il body

    sprintf(request_buffer, 
        "%s %s HTTP/1.0\r\n"
        "Host: %s\r\n"
        "User-Agent: esp-idf/1.0 esp32\r\n"
        "content-type: application/json\r\n"
        "Authorization: Basic dGVsZWNvOnRtYXRlMjA=\r\n"
        "content-length:%d\r\n"
        "\r\n"
        "%s\r\n"
        ,request_instance.method,request_instance.web_path, request_instance.web_host, strlen(request_instance.body), request_instance.body);

    Serial.println("Updating db");
    Serial.print("Sending httpRequest to: ");
    Serial.println((request_instance).web_host);

    // Use WiFiClient class to create TCP connections
    WiFiClient client;
    const int httpPort = 80;
    if (!client.connect((request_instance).web_host, httpPort)) {
        Serial.println("connection failed");
        if(WiFi.status() != WL_CONNECTED){
           initialise_wifi();
        }
        return;
    }

    /*
    // We now create a URI for the request
    String url = "/services/beacontrace/feedtemp";
    */

    Serial.print("Requesting URL: ");
    Serial.println((request_instance).web_path);

    // This will send the request to the server
    Serial.println(request_buffer);
    client.print(request_buffer);
    unsigned long timeout = millis();
    while (client.available() == 0) {
        if (millis() - timeout > 5000) {
            Serial.println(">>> Client Timeout !");
            client.stop();
            return;
        }
    }

    String response_s = "";
    // Read all the lines of the reply from server and print them to Serial
    while(client.available()) {
        String line = client.readStringUntil('\r');
        response_s += line;
    }


    /* Read HTTP response */
    char* recv_buf = &response_s[0];
    char response[256];
    char prev = recv_buf[0];
    int start_index = 0;
    int newline_count = 0;

    for(int i = 1; i < response_s.length(); i++) {
        char curr = recv_buf[i];
        if(curr == '\n' && prev == '\r'){
            newline_count++;
            if(newline_count == 2){
                start_index = i;
                break;
            }
        } 
        if(curr != '\n' && curr != '\r')
            newline_count = 0;
        prev = curr;
    }
    for(int a = 0; a + start_index < (response_s.length()); a++){
        response[a] = recv_buf[a + start_index];
    }

    response_callback(response);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    vTaskDelete(NULL);
}

void get_response(request_instance_t * instance, void (*callBack)(char * response)) {
    response_callback = callBack;
    request_instance = *instance;
    xTaskCreate(&http_get_task, "http_get_task", 8000, instance, 5, NULL);
}


void start_http_task()
{
    
    //ESP_ERROR_CHECK( nvs_flash_init() );
    
    initialise_wifi();
}