#include <U8x8lib.h>
#include <unordered_map>
#include <vector>
#include "LocationProtocol.h"
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
#define SERVER_UPDATE_INTERVAL_SECONDS 120
#define RAM_REF_INTERVAL_MILLIS 1000
#define BEACON_TIMEOUT_SECONDS 120

// comment if device has no display
#define HAS_DISPLAY

struct scan_result {
  ibeacon_instance_t beacon;
  unsigned long timestamp;
};

typedef std::vector<scan_result> ScansCollection;
typedef std::unordered_map<uint64_t, ScansCollection> DevMap;
DevMap* scansMap;
DevMap* oldMap;

// the OLED used
U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(/* clock=*/15, /* data=*/4,
                                       /* reset=*/16);

#ifdef HAS_DISPLAY
#define U8X8_POINTER() &u8x8
#else
#define U8X8_POINTER() NULL
#endif

MacAddress address;

request_instance_t requestUpdate = {
    "messages.geisoft.org", "/services/beacontrace/feedposition", "", "POST"};

unsigned long nodeLastCollectionSent;
unsigned long lastRamRef;

void setup() {
  initSPISerialAndDisplay();
  init_nvs();
  u8x8.drawString(0, 7, (String(esp_get_free_heap_size()) + " B").c_str());
  // read old configs from memory
  config_params_t config_params = readConfigsFromMemory();
  Serial.println(config_params.type);
  // codice per il softAP
  readMacAddress();
  setDeviceMode(APP_PURPOSES::LOCATION);
  vTaskStartScheduler();
  init(&config_params, address.toString(), U8X8_POINTER());
  config_params_t* user_params = clientListener();
// qui ho le config giuste
#ifdef HAS_DISPLAY
  u8x8.clearDisplay();
#endif
  if (user_params->type == DEVICE_TYPE_INVALID) {
    current_configs->type = DEVICE_TYPE_INVALID;
    u8x8.clearLine(1);
    u8x8.drawString(0, 1, "Reboot required");
    return;
  }
  initWithConfigParams(user_params, U8X8_POINTER(), true);  // dopo ci  va true

  if (current_configs->type != DEVICE_TYPE_AUTONOMOUS_TERMOMETER)
    initLoRa(address, SS, RST, DI0);
  else
    myAddress = address;

  if (current_configs->type != DEVICE_TYPE_TERMOMETER) {
    initHTTPTask();
    initTimeSync(U8X8_POINTER());
    subscribeToReceivePacketEvent(handleResponsePacket);
    scansMap = new DevMap();
    oldMap = new DevMap();
  }

  // ble init

  if (user_params->type != DEVICE_TYPE_NODE) {
    nvs_flash_init();
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    Serial.println("in");
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

#ifdef HAS_DISPLAY
  u8x8.clearDisplay();
  u8x8.drawString(0, 1, "Ready");
  printMACAddressToScreen(6);
#endif

  delay(1000);
  nodeLastCollectionSent = seconds();
}

void initSPISerialAndDisplay() {
  SPI.begin(5, 19, 27, 18);
#ifdef HAS_DISPLAY
  u8x8.begin();
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.drawString(0, 1, "Initializing");
#endif
  Serial.begin(115200);
}

void readMacAddress() {
  uint8_t mac[6];
  esp_efuse_mac_get_default(mac);
  Serial.println("initializing mac");
  address = MacAddress(mac);
  Serial.println(address.toString());
}

void loop() {
  if (esp_get_free_heap_size() < 5000) ESP.restart();  // evitimo un crash
  if (current_configs->type == DEVICE_TYPE_INVALID) {
    delay(1000);
  } else {
#ifdef HAS_DISPLAY
    if (millis() - lastRamRef >= RAM_REF_INTERVAL_MILLIS) {
      u8x8.drawString(0, 4, "Free RAM");
      u8x8.clearLine(5);
      delay(10);
      u8x8.drawString(0, 5, (String(esp_get_free_heap_size()) + " B").c_str());
      lastRamRef = millis();
    }
#endif

    if (current_configs->type != DEVICE_TYPE_TERMOMETER) {
      if (seconds() - nodeLastCollectionSent >=
          SERVER_UPDATE_INTERVAL_SECONDS) {
        Serial.println("");
        sendCollectionToServer();
        nodeLastCollectionSent = seconds();
      }
      // Serial.println((int)seconds() - (int)nodeLastCollectionSent);
    }
    if (current_configs->type == DEVICE_TYPE_NODE) checkIncoming();
    delay(10);
  }
  delay(100);
}

void handleResponsePacket(Packet packet) {
  if (isLocationScanPacket(packet.type, packet.packetLength)) {
    // readTempAndSendToServerIfNecessary(&packet);
    packet.printPacket();
    distanceScanCompletedCallback(
        packet.sender, decodeBeaconFromPacket((uint8_t*)packet.body));
  }
  Serial.println("packet");
}

void printMACAddressToScreen(int baseRow) {
  char* mc = address.toCharArray();
  u8x8.drawString(0, baseRow, "MAC");
  u8x8.clearLine(baseRow + 1);
  u8x8.drawString(0, baseRow + 1, mc);
  DELETE_ARRAY(mc)
}

void ble_ibeacon_init(void) {
  esp_bluedroid_init();
  esp_bluedroid_enable();
  ble_ibeacon_appRegister(distanceScanCompletedCallback);
}
