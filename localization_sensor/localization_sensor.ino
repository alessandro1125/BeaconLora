#include <unordered_map>
#include <vector>
#include "LocationProtocol.h"
#include "disp_helper.h"
#include "esp_bt.h"
#include "esp_system.h"
#include "ibeacon_message_handler.h"
#include "init_configs_tools.h"
#include "time_sync.h"
#include "wifi_config.h"

#define SS 18
#define RST 14
#define DI0 26
#define BAND 868E6

#define seconds() (millis() / 1000)
#define DIST_ACCEPTANCE_INTERVAL 1.0
#define SERVER_UPDATE_INTERVAL_SECONDS 60
#define RAM_REF_INTERVAL_MILLIS 1000

// comment if device has no display
#define HAS_DISPLAY

struct scan_result {
  ibeacon_instance_t beacon;
  unsigned long timestamp;
};

typedef std::vector<scan_result> ScansCollection;
typedef std::unordered_map<uint64_t, ScansCollection> DevMap;
DevMap scansMap;
DevMap oldMap;

#ifdef HAS_DISPLAY
#define U8X8_POINTER() &u8g2
#else
#define U8X8_POINTER() &display
#endif

MacAddress address;

request_instance_t requestUpdate = {
    "messages.geisoft.org", "/services/beacontrace/feedposition", "", "POST"};

unsigned long nodeLastCollectionSent;
unsigned long lastRamRef;

U8G2_SSD1306_128X64_NONAME_1_SW_I2C u8g2(U8G2_R0, /* clock=*/15, /* data=*/4,
                                         /* reset=*/16);

static Display display;

void setup() {
  pinMode(16, OUTPUT);
  digitalWrite(16, LOW);  // set GPIO16 low to reset OLED
  delay(50);
  digitalWrite(16, HIGH);  // while OLED is running, must set GPIO16 in high
  initSPISerialAndDisplay();
  init_nvs();

  display.setRow(7, String(ESP.getFreeHeap()).c_str());
  display.refresh();
  // read old configs from memory
  config_params_t config_params = readConfigsFromMemory();
  Serial.println("configs read");
  // codice per il softAP
  readMacAddress();

  wifi_config wifiConfig = wifi_config();
  Serial.println("mac address read");
  wifiConfig.setDeviceMode(APP_PURPOSES::LOCATION);
  Serial.println("Purpose set");
  vTaskStartScheduler();
  delay(10);
  Serial.println("scheduler started");

  char* addrch = address.toCharArray();
  wifiConfig.init(&config_params, addrch, &display);
  DELETE_ARRAY(addrch);
  config_params_t* user_params = wifiConfig.clientListener();
  // qui ho le config giuste
  display.clear();
  if (user_params->type == DEVICE_TYPE_INVALID) {
    current_configs->type = DEVICE_TYPE_INVALID;
    display.setRow(1, "Reboot ");
    display.refresh();
    return;
  }
  initWithConfigParams(user_params, &display, true);  // dopo ci  va true

  if (current_configs->type != DEVICE_TYPE_AUTONOMOUS_TERMOMETER)
    initLoRa(address, SS, RST, DI0);
  else
    myAddress = address;
  if (current_configs->type == DEVICE_TYPE_NODE)
    subscribeToReceivePacketEvent(handleResponsePacket);

  if (current_configs->type != DEVICE_TYPE_TERMOMETER) {
    initHTTPTask();
    initTimeSync(&display);
  }
  // ble init

  if (user_params->type != DEVICE_TYPE_NODE) {
    nvs_flash_init();
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));
    ble_ibeacon_init();
  }

  if (user_params->bt_configs.referencePower != 0 &&
      user_params->bt_configs.noise != 0)
    updateRSSIParams(user_params->bt_configs.referencePower,
                     user_params->bt_configs.noise != 0);
  // end ble init

  display.clear();
  display.setRow(1, "Ready");
  display.refresh();
  printMACAddressToScreen(6);
  nodeLastCollectionSent = seconds();
  // delete user_params;  // wifi config mi ritorna una nuova istanza,
  // initWithConfigParams se ne fa una copia e questa resta
  // in più
}

void loop() {
  delay(10);
  display.setRow(1, "Ready");
  if (esp_get_free_heap_size() < 5000) ESP.restart();  // evitiamo un crash
  if (current_configs->type == DEVICE_TYPE_INVALID) {
    delay(1000);
  } else {
    if (millis() - lastRamRef >= RAM_REF_INTERVAL_MILLIS) {
      display.setRow(4, "Free Ram");
      delay(10);
      const char* ram = String(ESP.getFreeHeap()).c_str();
      Serial.println(ram);
      display.setRow(5, ram);
      printMACAddressToScreen(6);
      lastRamRef = millis();
    }

    if (current_configs->type != DEVICE_TYPE_TERMOMETER) {
      if (seconds() - nodeLastCollectionSent >=
          SERVER_UPDATE_INTERVAL_SECONDS) {
        if (WiFi.status() != WL_CONNECTED)
          connectToWifi(current_configs->wifi_configs, &display, false);
        sendCollectionToServer();
        nodeLastCollectionSent = seconds();
        display.clear();
      }
    }
    if (current_configs->type == DEVICE_TYPE_NODE) checkIncoming();
    delay(10);
  }
  delay(100);
  display.refresh();
}

void initSPISerialAndDisplay() {
  SPI.begin(5, 19, 27, 18);
#ifdef HAS_DISPLAY
  display.init(U8X8_POINTER(), true);  // false se non ha il display
  display.setRow(1, "Initializing");
  display.refresh();
#endif
  Serial.begin(115200);
}

void readMacAddress() {
  uint8_t mac[6];
  esp_efuse_mac_get_default(mac);
  address = MacAddress(mac);
}

void handleResponsePacket(Packet packet) {
  if (isLocationScanPacket(packet.type, packet.packetLength)) {
    // readTempAndSendToServerIfNecessary(&packet);
    packet.printPacket();
    distanceScanCompletedCallback(
        packet.sender, decodeBeaconFromPacket((uint8_t*)packet.body));
  }
}

void printMACAddressToScreen(int baseRow) {
  char* mc = address.toCharArray();
  display.setRow(baseRow, "MAC");
  display.setRow(baseRow + 1, mc);
  DELETE_ARRAY(mc);
}

void ble_ibeacon_init(void) {
  esp_bluedroid_init();
  esp_bluedroid_enable();
  ble_ibeacon_appRegister(distanceScanCompletedCallback);
}
