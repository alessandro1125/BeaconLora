#include <U8x8lib.h>
#include "ibeacon_message_handler.h"
#include "LoRaProtocol.h"
//#include "time_sync.h"

#define SS      18
#define RST     14
#define DI0     26
#define BAND    868E6

U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(/* clock=*/ 15, /* data=*/ 4, /* reset=*/ 16);

void setup() {

  u8x8.begin();
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.drawString(0, 1, "Initializing");
  Serial.begin(115200);
  
  uint8_t mac[6];
  esp_efuse_mac_get_default(mac);
  Serial.println("initializing mac");
  address = MacAddress(mac);
  Serial.println(address.toString());

  node = MacAddress((uint64_t)0);
  
  initLoRa(address, SS, RST, DI0);

  //initTimeSync(&u8x8);  

  ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
  esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  esp_bt_controller_init(&bt_cfg);
  esp_bt_controller_enable(ESP_BT_MODE_BLE);
  ble_ibeacon_init();
  

}

void ble_ibeacon_init(void)
{
    esp_bluedroid_init();
    esp_bluedroid_enable();
    ble_ibeacon_appRegister();
}

void loop() {
  // put your main code here, to run repeatedly:

}

void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    esp_err_t err;

    switch (event) {
        case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:{
          break;
        }
        case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
            //the unit of the duration is second, 0 means scan permanently
            printf("scan param set complete\n");
            uint32_t duration = 0;
            esp_ble_gap_start_scanning(duration);
            break;
        }
        case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
            //scan start complete event to indicate scan start successfully or failed
            if ((err = param->scan_start_cmpl.status) != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(DEMO_TAG, "Scan start failed: %s", esp_err_to_name(err));
            }
            break;
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
      ESP_LOGI(DEMO_TAG, "Start adv completed");
            //adv start complete event to indicate adv start successfully or failed
            if ((err = param->adv_start_cmpl.status) != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(DEMO_TAG, "Adv start failed: %s", esp_err_to_name(err));
            }
            break;
        case ESP_GAP_BLE_SCAN_RESULT_EVT: {
            esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;
            //gestisce il pacchetto in arrivo
            handle_received_packet( scan_result, ibeacon_scanned_list );
            break;
        }

        case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
            if ((err = param->scan_stop_cmpl.status) != ESP_BT_STATUS_SUCCESS){
                ESP_LOGE(DEMO_TAG, "Scan stop failed: %s", esp_err_to_name(err));
            }
            else {
                ESP_LOGI(DEMO_TAG, "Stop scan successfully");
            }
            break;

        case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
            break;

        default:
            break;
    }
}
