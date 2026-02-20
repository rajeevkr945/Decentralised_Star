/*
 * ble_adv_ext.c
 * Extended Advertising Module (Reverting to Legacy Advertising as per request)
 * The filename is kept for project consistency but implementation uses Legacy
 * APIs.
 */

#include "ble_adv_ext.h"
#include "ble_common.h"
#include "esp_log.h"
#include "esp_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

#define ADV_TAG "BLE_ADV_LEGACY"

// Legacy Advertising Parameters
static esp_ble_adv_params_t adv_params = {
    .adv_int_min = 0x64, // 100ms
    .adv_int_max = 0x64,
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

static uint8_t raw_adv_data[] = {
    // Flags
    0x02, 0x01, 0x06,
    // Name
    0x0d, 0x09, 'M', 'O', 'D', 'U', 'L', 'A', 'R', '_', 'N', 'O', 'D', 'E',
    // Manufacturer Data Place Holder (will be updated)
    0x05, 0xff, 0xff, 0xff, 0x00, 0x00};

static TaskHandle_t s_adv_task_handle = NULL;
static bool s_adv_initialized = false;

void ble_adv_update_task(void *pvParameters)
{
  ESP_LOGI(ADV_TAG, "Adv update task started");

  uint8_t *mfg_data_ptr =
      &raw_adv_data[sizeof(raw_adv_data) - 4]; // Last 4 bytes

  while (1)
  {
    vTaskDelay(pdMS_TO_TICKS(BLE_ADV_UPDATE_INTERVAL_MS));

    if (!s_adv_initialized)
      continue;

    // Generate fake sensor data
    uint16_t sensor_val = (uint16_t)esp_random();
    mfg_data_ptr[2] = (sensor_val >> 8) & 0xFF;
    mfg_data_ptr[3] = sensor_val & 0xFF;

    ESP_LOGI(ADV_TAG, "Updating Adv Data: SensorVal=0x%04x", sensor_val);

    // Update raw data
    esp_err_t ret =
        esp_ble_gap_config_adv_data_raw(raw_adv_data, sizeof(raw_adv_data));
    if (ret != ESP_OK)
    {
      ESP_LOGE(ADV_TAG, "Failed to update adv data: %s", esp_err_to_name(ret));
    }
  }
}

void ble_adv_init(void)
{
  // No "Set Params" equivalent that triggers an event in Legacy (Start triggers
  // it). But config_adv_data needs to happen. For legacy, we can config data
  // first or anytime.

  // We initiate the update task
  s_adv_initialized = true;
  xTaskCreate(ble_adv_update_task, "ble_adv_update", BLE_ADV_TASK_STACK_SIZE,
              NULL, BLE_ADV_TASK_PRIORITY, &s_adv_task_handle);
}

void ble_adv_config_data(void)
{
  esp_err_t ret =
      esp_ble_gap_config_adv_data_raw(raw_adv_data, sizeof(raw_adv_data));
  if (ret != ESP_OK)
  {
    ESP_LOGE(ADV_TAG, "Failed to config adv data: %s", esp_err_to_name(ret));
  }
}

void ble_adv_start(void)
{
  esp_err_t ret = esp_ble_gap_start_advertising(&adv_params);
  if (ret != ESP_OK)
  {
    ESP_LOGE(ADV_TAG, "Failed to start legacy advertising: %s",
             esp_err_to_name(ret));
  }
}
