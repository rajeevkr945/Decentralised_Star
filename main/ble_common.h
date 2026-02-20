/*
 * ble_common.h
 * Common types and configurations for Modular BLE System
 */

#ifndef BLE_COMMON_H
#define BLE_COMMON_H

#include "esp_gap_ble_api.h"
#include "esp_gatt_defs.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------- */
/*                               Configuration                                */
/* -------------------------------------------------------------------------- */

#define BLE_SCAN_DURATION_SEC 5
#define BLE_SCAN_WINDOW_MS 100
#define BLE_SCAN_INTERVAL_MS 200
#define BLE_SCAN_TASK_STACK_SIZE 4096
#define BLE_SCAN_TASK_PRIORITY 5

#define BLE_ADV_UPDATE_INTERVAL_MS 5000
#define BLE_ADV_TASK_STACK_SIZE 3072
#define BLE_ADV_TASK_PRIORITY 4

#define BLE_FILTER_NAME_STR "MY_TARGET_DEVICE"

/* -------------------------------------------------------------------------- */
/*                                   Types                                    */
/* -------------------------------------------------------------------------- */

/**
 * @brief Parsed Advertisement Record
 */
typedef struct {
  uint8_t addr[6];
  esp_ble_addr_type_t addr_type;
  int rssi;

  // Parsed fields
  char device_name[33]; // Null-terminated, max 32 chars
  uint8_t mfg_data[32]; // Raw manufacturer data
  uint8_t mfg_data_len;
  uint8_t flags;
  bool has_name;
  bool has_mfg_data;
} ble_adv_record_t;

/**
 * @brief Scan Configuration Structure
 */
typedef struct {
  uint16_t scan_window;
  uint16_t scan_interval;
  uint32_t duration_sec;
  bool active_scan;
} ble_scan_config_t;

/**
 * @brief Filter Function Prototype
 * @return true if record passes filter, false otherwise
 */
typedef bool (*ble_adv_filter_fn)(const ble_adv_record_t *record);

#ifdef __cplusplus
}
#endif

#endif // BLE_COMMON_H
