//#include <Arduino.h>
#include <FreeRTOS.h>
#include <mutex>  // std::mutex
#include <unordered_map>
#include <vector>
#include "LocationProtocol.h"
#include "disp_helper.h"
#include "esp_bt.h"
#include "esp_system.h"
#include "ibeacon_message_handler.h"
#include "init_configs_tools.h"
#include "loop_watchdog.h"
#include "time_sync.h"
#include "wifi_config.h"

#define SS 18
#define RST 14
#define DI0 26
#define BAND 868E6

#define seconds() (millis() / 1000)
#define DIST_ACCEPTANCE_INTERVAL 1.0
#define SERVER_UPDATE_INTERVAL_SECONDS 30
#define RAM_REF_INTERVAL_MILLIS 5000

// comment if device has no display
#define HAS_DISPLAY

typedef std::vector<BeaconInfo> ScansCollection;
typedef std::unordered_map<uint64_t, ScansCollection> DevMap;

DevMap scansMap;  // mappa che conterrà le scansioni dei beacon
DevMap oldMap;  // mappa che verrà utilizzata per l'invio dei dati al server in
                // modo da lasciare scansMap libera di essere utilizzata nel
                // frattempo

#ifdef HAS_DISPLAY
#define U8X8_POINTER() &u8g2
#else
#define U8X8_POINTER() &display
#endif

MacAddress address;  // indirizzo mac di questo dispositivo

request_instance_t requestUpdate =
    request_instance_t(String("messages.geisoft.org"),
                       String("/services/beacontrace/feedposition"),
                       (String*)NULL, String("POST"));

unsigned long nodeLastCollectionSent;
unsigned long lastRamRef;

U8G2_SSD1306_128X64_NONAME_1_SW_I2C u8g2(U8G2_R0, /* clock=*/15, /* data=*/4,
                                         /* reset=*/16);

static Display display;

static int timeoutCount =
    0;  // tengo il conto delle volte consecutive in cui è andata in timeout la
        // connessione http. Se la connessione va in timeout i dati che dovevano
        // essere inviati non vengono eliminati per poter essere reinviati. Se
        // va in timeout troppe volte di seguito la stringa di request htto
        // diventa troppo grande per essere contenuta in memoria e questo
        // innesca un circolo vizioso in cui il request inviato è vuoto e quidi
        // il client va sempre in timeout e quindi genera richieste sempre più
        // grandi che ovviamente poi non riesce ad inviare. Per evitare tali
        // situazioni tengo il conto di quante volte consecutive sono andato in
        // timeout e se ne ho più di 2 elimino i dati che altrimenti perderei
        // comunque per non perdere anche un numero indefinito di dati
        // successivi. Si spera che questo non debba mai succedere ma dato che
        // il pericolo che succeda è reale questa logica viene mantenuta.

std::mutex mutex;  // mutex per evitare accessi alla risorsa scansMap

LoopWatchdog watchdog;

void setup() {
  pinMode(16, OUTPUT);
  digitalWrite(16, LOW);  // set GPIO16 low to reset OLED
  delay(50);
  digitalWrite(16, HIGH);  // while OLED is running, must set GPIO16 in high
  initSPISerialAndDisplay();
  delay(10);
  Serial.println(F("Serial initialized"));
  init_nvs();

  Serial.println(F("nvs initialized"));
  delay(100);
  display.setRow(7, String(ESP.getFreeHeap()).c_str());
  display.refresh();

  //>>>>>>>> CONFIGURAZIONI UTENTE VIA WIFI
  config_params_t config_params = readConfigsFromMemory();
  Serial.println(F("configs read"));
  readMacAddress();

  wifi_config wifiConfig = wifi_config();
  Serial.println(F("mac address read"));
  wifiConfig.setDeviceMode(APP_PURPOSES::LOCATION);
  Serial.println(F("Purpose set"));

  std::unique_ptr<char[]> addrch = address.toCharArray();
  Serial.println(String(addrch.get()));
  wifiConfig.init(&config_params, addrch.get(), &display);
  // DELETE_ARRAY(addrch);
  config_params_t* user_params = wifiConfig.clientListener();
  display.clear();
  if (user_params->type == DeviceType::InvalidDevice) {
    current_configs.lockParams();
    current_configs.getParams()->type = DeviceType::InvalidDevice;
    current_configs.unlockParams();
    display.setRow(1, "Reboot ");
    display.refresh();
    return;
  }
  initWithConfigParams(user_params, &display, true);
  //<<<<<<<< FINE

  //>>>>>>>> INIZIALIZZAZIONE LORA
  if (current_configs.getType() != DeviceType::AutonomousSensor)
    Lora::initLoRa(address, SS, RST, DI0);
  else
    Lora::myAddress = address;
  if (current_configs.getType() == DeviceType::Node)
    Lora::subscribeToReceivePacketEvent(handleResponsePacket);
  //<<<<<<<< FINE

  //>>>>>>>> INIZIALIZZAZIONE HTTP
  if (current_configs.getType() != DeviceType::Sensor) {
    initHTTPTask(APP_PURPOSES::LOCATION);
    initTimeSync(timeSyncedCallback);
  }
  //<<<<<<<< FINE

  //>>>>>>>> INIZIALIZZAZIONE BLUETOOTH

  if (user_params->bt_configs.referencePower != 0 &&
      user_params->bt_configs.noise != 0)
    updateRSSIParams(user_params->bt_configs.referencePower,
                     user_params->bt_configs.noise);

  if (user_params->type != DeviceType::Node) {
    nvs_flash_init();
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));
    ble_ibeacon_init();
  }

  //<<<<<<<<< FINE

  display.clear();
  display.setRow(1, "Attivo");
  printMACAddressToScreen(6);
  display.refresh();
  nodeLastCollectionSent = seconds();
}

void timeSyncedCallback(bool valid) {
  if (!valid) {
    display.clear();
    display.setRow(1, "TIME SYNC:");
    display.setRow(2, "FAILED");
    display.setRow(3, "Reboot in 10s");
    display.refresh();
    delay(10000);

    ESP.restart();
  }
  watchdog.resetWatchdog();  // starting the watchdog
                             // controlling hangings of looptask
  watchdog.startWatchdog();
  if (current_configs.getType() == DeviceType::Node)
    xTaskCreate(loraLoopTask, "loraLoop", 5000, NULL, 5, NULL);
}

void loraLoopTask(void* vParameters) {
  for (;;) {
    Lora::checkIncoming();
    delay(5);
  }
  vTaskDelete(NULL);
}

void loop() {
  watchdog.resetWatchdog();
  if (ESP.getFreeHeap() < 15000)
    ESP.restart();  // evitiamo un crash, resta piantato altrimenti
  if (current_configs.getType() == DeviceType::InvalidDevice) {
    delay(1000);
  } else {
    if (millis() - lastRamRef >= RAM_REF_INTERVAL_MILLIS) {
      display.setRow(4, "Ram libera");
      // delay(10);
      const char* ram = String(ESP.getFreeHeap()).c_str();
      Serial.print(F("Free ram: "));
      Serial.println(ram);
      display.setRow(5, ram);
      printMACAddressToScreen(6);
      lastRamRef = millis();
    }

    if (current_configs.getType() != DeviceType::Sensor) {
      if (seconds() - nodeLastCollectionSent >=
          SERVER_UPDATE_INTERVAL_SECONDS) {
        display.clear();
        if (WiFi.status() != WL_CONNECTED) {
          Serial.println(F("Reconnectiong"));
          display.setRow(3, "Riconnessione");
          current_configs.lockParams();
          connectToWifi(current_configs.getParams()->wifi_configs, &display,
                        false);
          current_configs.unlockParams();
        } else
          display.setRow(1, "Attivo");
        sendCollectionToServer();
        nodeLastCollectionSent = seconds();
      }
    }
  }
  display.refresh();

  if (current_configs.getType() == DeviceType::AutonomousSensor)
    delay(5000);  // posso andare proprio tranquillo con il
                  // delay così lascio spazio al bluetooth
  /*else
    delay(50);*/
}

void initSPISerialAndDisplay() {
  SPI.begin(5, 19, 27, 18);
#ifdef HAS_DISPLAY
  display.init(U8X8_POINTER(), true);  // false se non ha il display
  display.setRow(1, "Inizializzo");
  display.refresh();
#else
  dispaly.init(U8X8_POINTER(), false);
#endif
  Serial.begin(115200);
}

void readMacAddress() {
  uint8_t mac[6];
  esp_efuse_mac_get_default(mac);
  address = MacAddress(mac);
}

void handleResponsePacket(const Lora::Packet& packet) {
  display.setRow(3, packet.sender.toString().c_str());
  if (isLocationScanPacket(packet.type, packet.packetLength)) {
    Serial.print(F("Packet received "));
    Serial.println(packet.packetNumber);
    distanceScanCompletedCallback(
        packet.sender, decodeBeaconFromPacket((uint8_t*)packet.body));
  }
}

void printMACAddressToScreen(int baseRow) {
  std::unique_ptr<char[]> mc = address.toCharArray();
  display.setRow(baseRow, "MAC");
  display.setRow(baseRow + 1, mc.get());
  // DELETE_ARRAY(mc);
}

void ble_ibeacon_init(void) {
  esp_bluedroid_init();
  esp_bluedroid_enable();
  ble_ibeacon_appRegister(distanceScanCompletedCallback);
}
