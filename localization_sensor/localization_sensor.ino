#include <U8x8lib.h>
#include "LocationProtocol.h"
#include "init_configs_tools.h"
//#include "esp_system.h"
#include "time_sync.h"
#include "wifi_config.h"
#include "ibeacon_message_handler.h"

#define SS      18
#define RST     14
#define DI0     26
#define BAND    868E6

#define HELTEC_WIFI_LORA_32_BT
//#define ESP_WROOM_32

#define seconds() (millis()/1000)
#define TERM_SCAN_INTERVAL_MILLIS 1000
#define TEMP_ACCEPTANCE_INTERVAL 0.05
#define SERVER_UPDATE_INTERVAL_SECONDS 60
#define RAM_REF_INTERVAL_MILLIS 1000

// the OLED used

#ifdef HELTEC_WIFI_LORA_32_BT
U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(/* clock=*/ 15, /* data=*/ 4, /* reset=*/ 16);
#endif

#ifdef HELTEC_WIFI_LORA_32_BT
#define U8X8_POINTER() &u8x8
#endif
#ifdef ESP_WROOM_32
#define U8X8_POINTER() NULL
#endif

MacAddress address;

request_instance_t requestUpdate = { "messages.geisoft.org" , "/services/beacontrace/feedtemp" , "" , "POST"};

//std::unordered_map<uint64_t, float> tempMap;

unsigned long autonomousTermometerLastSensorRead;
unsigned long nodeLastCollectionSent;
unsigned long lastRamRef;

void setup() {
  
  initSPISerialAndDisplay();
  init_nvs();
  //read old configs from memory
  config_params_t config_params = readConfigsFromMemory();
  Serial.println(config_params.type);
  
  //codice per il softAP
  readMacAddress();
  init(&config_params, address.toString(), U8X8_POINTER());
  config_params_t * user_params = clientListener();
  //qui ho le config giuste
  #ifdef HELTEC_WIFI_LORA_32_BT
  u8x8.clearDisplay();
  #endif
  if(user_params->type == DEVICE_TYPE_INVALID){
    deviceType = DEVICE_TYPE_INVALID;
    u8x8.clearLine(1);
    u8x8.drawString(0, 1, "Reboot required");
    return;
  }
  initWithConfigParams(user_params, U8X8_POINTER(), true);// dopo ci  va true
  initLoRa(address, SS, RST, DI0);

  if(deviceType != DEVICE_TYPE_TERMOMETER){
    initTimeSync(U8X8_POINTER());
    subscribeToReceivePacketEvent(handleResponsePacket);
  }
  
  //ble init

  if(user_params->type != DEVICE_TYPE_NODE){
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_bt_controller_init(&bt_cfg);
    esp_bt_controller_enable(ESP_BT_MODE_BLE);
    ble_ibeacon_init();
  }
  //end ble init

  #ifdef HELTEC_WIFI_LORA_32_BT
  u8x8.clearLine(1);
  u8x8.drawString(0, 1, "Ready");
  printMACAddressToScreen(6);
  #endif
  
  delay(1000);  
}

void initSPISerialAndDisplay(){
  SPI.begin(5, 19, 27, 18);
  #ifdef HELTEC_WIFI_LORA_32_BT
  u8x8.begin();
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.drawString(0, 1, "Initializing");
  #endif
  Serial.begin(115200);
}

void readMacAddress(){
  uint8_t mac[6];
  esp_efuse_mac_get_default(mac);
  Serial.println("initializing mac");
  address = MacAddress(mac);
  Serial.println(address.toString());
  
}

void loop() {
  if(deviceType == DEVICE_TYPE_INVALID){
    delay(1000);
  } else {
    
    if(deviceType != DEVICE_TYPE_TERMOMETER){
      checkIncoming();
      /*if(seconds() - nodeLastCollectionSent >= SERVER_UPDATE_INTERVAL_SECONDS){
        Serial.println("before sendCollection " + (String(esp_get_free_heap_size()) + " B"));
        sendCollectionToServer();
        
        nodeLastCollectionSent = seconds();
      }*/
      #ifdef HELTEC_WIFI_LORA_32_BT
        if(millis() - lastRamRef >= RAM_REF_INTERVAL_MILLIS){
          u8x8.drawString(0, 4 , "Free RAM");
          u8x8.clearLine(5);
          u8x8.drawString(0, 5 , (String(esp_get_free_heap_size()) + " B").c_str());
          lastRamRef = millis();
        }
      #endif
      
      delay(10);
    }
    
    /*if(deviceType != DEVICE_TYPE_NODE){
       float temperature = getTemp();
       if(temperature != -1000 && temperature != 85){
        if(deviceType == DEVICE_TYPE_TERMOMETER){
          u8x8.drawString(0, 4 , "Free RAM");
          u8x8.clearLine(5);
          u8x8.drawString(0, 5 , (String(esp_get_free_heap_size()) + " B").c_str());
          sendLoraTemperaturePacket(temperature);
        }
        if(deviceType == DEVICE_TYPE_AUTONOMOUS_TERMOMETER && (millis() - autonomousTermometerLastSensorRead >= TERM_SCAN_INTERVAL_MILLIS) ){
          addScanToCollection(address, temperature, getCurrentTime());
          autonomousTermometerLastSensorRead = millis();
        }
       }
    }*/
  }
}

void handleResponsePacket(Packet packet) {
    if(isLocationScanPacket(packet.type, packet.packetLength)){
       //readTempAndSendToServerIfNecessary(&packet);
       packet.printPacket();
       distanceScanCompletedCallback(&decodeBeaconFromPacket(packet.data()));
    }
    Serial.println("packet");
}


void printMACAddressToScreen(int baseRow){
  char* mc = address.toCharArray();
  u8x8.drawString(0, baseRow , "MAC");
  u8x8.clearLine(baseRow + 1);
  u8x8.drawString(0, baseRow + 1, mc);
  DELETE_ARRAY(mc)
}

void ble_ibeacon_init(void)
{
    esp_bluedroid_init();
    esp_bluedroid_enable();
    ble_ibeacon_appRegister();
}




