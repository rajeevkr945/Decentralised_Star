/*
 * main.c
 * System Entry Point for Modular BLE System
 */

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_err.h"
#include "esp_gap_ble_api.h"
#include "esp_gatt_common_api.h"
#include "esp_log.h"
#include "esp_random.h"
#include "nvs_flash.h"

#include "ble_adv_ext.h"
#include "ble_common.h"
#include "ble_gatts.h"
#include "ble_scan.h"

#define TAG "MAIN"

static bool s_scanner_initialized = false;

static void gap_event_handler(esp_gap_ble_cb_event_t event,
                              esp_ble_gap_cb_param_t *param) {
  switch (event) {
  case ESP_GAP_BLE_SCAN_RESULT_EVT:
    ble_scan_process_result(param);
    break;

  case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:
    ESP_LOGI(TAG, "Scan param set complete");
    break;

  case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
    if (param->scan_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
      ESP_LOGE(TAG, "Scan start failed: %d", param->scan_start_cmpl.status);
    }
    break;

  case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
    ESP_LOGD(TAG, "Scan stop complete");
    ble_scan_on_stop_complete();
    break;

  // Legacy Advertising Events
  case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
    ESP_LOGI(TAG, "Legacy Adv Data Raw set complete");
    // let's just call start.
    ble_adv_start();
    break;

  case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
    if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
      ESP_LOGE(TAG, "Legacy Adv start failed: %d",
               param->adv_start_cmpl.status);
      ESP_LOGE(TAG, "CRITICAL: Unable to start advertising. Halting system.");
      abort();
    } else {
      ESP_LOGI(TAG, "Legacy Adv Started successfully");
      if (!s_scanner_initialized) {
        ble_scan_init();
        s_scanner_initialized = true;
      }
    }
    break;

  default:
    ESP_LOGV(TAG, "GAP Event %d", event);
    break;
  }
}

void app_main(void) {
  esp_err_t ret;

  // 1. Initialize NVS
  ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  // 2. Release Classic BT memory
  ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

  // 3. Initialize Controller (BLE Only)
  esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  ret = esp_bt_controller_init(&bt_cfg);
  if (ret) {
    ESP_LOGE(TAG, "%s initialize controller failed: %s", __func__,
             esp_err_to_name(ret));
    return;
  }

  ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
  if (ret) {
    ESP_LOGE(TAG, "%s enable controller failed: %s", __func__,
             esp_err_to_name(ret));
    return;
  }

  // 4. Initialize Bluedroid Host
  ret = esp_bluedroid_init();
  if (ret) {
    ESP_LOGE(TAG, "%s init bluetooth failed: %s", __func__,
             esp_err_to_name(ret));
    return;
  }
  ret = esp_bluedroid_enable();
  if (ret) {
    ESP_LOGE(TAG, "%s enable bluetooth failed: %s", __func__,
             esp_err_to_name(ret));
    return;
  }

  // 5. Register Callbacks
  // 5. Register Callbacks
  ret = esp_ble_gap_register_callback(gap_event_handler);
  if (ret) {
    ESP_LOGE(TAG, "gap register error, error code = %x", ret);
    return;
  }

  // 5.1 Set Static Random Address (Required for BLE_ADDR_TYPE_RANDOM)
  esp_bd_addr_t rand_addr;
  esp_fill_random(rand_addr, sizeof(rand_addr));

  // Set the two most significant bits to 1 (Static Random Address)
  // Assuming big-endian representation for the bits logic here,
  // or simply ensuring it matches the Random Static requirement.
  // In ESP-IDF BDA, index 0 is often treated as MSB for display,
  // but for requirement "top 2 bits are 1", we usually set 0xC0 in the MSB
  // byte.
  rand_addr[0] |= 0xC0;

  ret = esp_ble_gap_set_rand_addr(rand_addr);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to set random address: %s", esp_err_to_name(ret));
    return;
  }
  ESP_LOGI(TAG, "Random Address Set: %02x:%02x:%02x:%02x:%02x:%02x",
           rand_addr[0], rand_addr[1], rand_addr[2], rand_addr[3], rand_addr[4],
           rand_addr[5]);

  // 6. Initialize Modules
  ESP_LOGI(TAG, "Initializing BLE Modules...");
  ble_gatts_init();      // Needs to register GATTS callbacks
  ble_adv_init();        // Sets up tasks
  ble_adv_config_data(); // Triggers the event chain: Data Set -> Start Adv ->
                         // Start Scan

  ESP_LOGI(TAG, "System Initialized");
}
