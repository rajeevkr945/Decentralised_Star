/*
 * ble_scan.c
 * Continuous Burst Scanning Module (Legacy Scanning)
 * Reverted to Legacy APIs for compatibility.
 */

#include "ble_scan.h"
#include "ble_adv_parser.h"
#include "ble_common.h"
#include "ble_scan_filter.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

#define SCAN_TAG "BLE_SCAN_LEGACY"

static TaskHandle_t s_scan_task_handle = NULL;
static bool s_scanning_enabled = false;

static esp_ble_scan_params_t scan_params = {
    .scan_type = BLE_SCAN_TYPE_PASSIVE,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval = 0x50, // 80 * 0.625ms = 50ms
    .scan_window = 0x30,   // 48 * 0.625ms = 30ms
    .scan_duplicate = BLE_SCAN_DUPLICATE_ENABLE};

void ble_scan_process_result(esp_ble_gap_cb_param_t *param)
{
  // Legacy Scan Result
  esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;
  switch (scan_result->scan_rst.search_evt)
  {
  case ESP_GAP_SEARCH_INQ_RES_EVT:
  {
    ble_adv_record_t record;
    memcpy(record.addr, scan_result->scan_rst.bda, 6);
    record.addr_type = scan_result->scan_rst.ble_addr_type;
    record.rssi = scan_result->scan_rst.rssi;

    // Parse data
    ble_parse_adv_data(scan_result->scan_rst.ble_adv,
                       scan_result->scan_rst.adv_data_len, &record);

    if (ble_filter_process(&record))
    {
      ESP_LOGI(SCAN_TAG,
               "MATCH: Name='%s' Addr=%02x:%02x:%02x:%02x:%02x:%02x RSSI=%d",
               record.has_name ? record.device_name : "(Hidden)",
               record.addr[0], record.addr[1], record.addr[2], record.addr[3],
               record.addr[4], record.addr[5], record.rssi);
    }
    break;
  }
  default:
    break;
  }
}

static void ble_scan_task_func(void *pvParameters)
{
  ESP_LOGI(SCAN_TAG, "Scan task started");

  while (1)
  {
    if (s_scanning_enabled)
    {
      // Burst Scan ON
      ESP_LOGD(SCAN_TAG, "Starting Burst Scan");
      // Duration in seconds (Legacy API takes seconds)
      esp_ble_gap_start_scanning(BLE_SCAN_DURATION_SEC);

      // Wait for duration + buffer
      vTaskDelay(pdMS_TO_TICKS(BLE_SCAN_DURATION_SEC * 1000 + 100));

      // Scan stops automatically after duration (if using duration)
      // Or we stop it manually if we used continuous.
      // esp_ble_gap_start_scanning(duration) stops automatically?
      // Yes, calls ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT.

      // We wait a bit before restarting.
      vTaskDelay(pdMS_TO_TICKS(100));
    }
    else
    {
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }
}

void ble_scan_init(void)
{
  esp_err_t ret = esp_ble_gap_set_scan_params(&scan_params);
  if (ret != ESP_OK)
  {
    ESP_LOGE(SCAN_TAG, "Failed to set scan params: %s", esp_err_to_name(ret));
    return;
  }

  s_scanning_enabled = true;
  xTaskCreate(ble_scan_task_func, "ble_scan_task", BLE_SCAN_TASK_STACK_SIZE,
              NULL, BLE_SCAN_TASK_PRIORITY, &s_scan_task_handle);
}

void ble_scan_on_stop_complete(void)
{
  ble_filter_clear_cache();
}
