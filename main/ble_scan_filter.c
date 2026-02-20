/*
 * ble_scan_filter.c
 * Scan Filter Framework & Implementation
 */

#include "ble_scan_filter.h"
#include "ble_common.h"
#include "esp_log.h"
#include <string.h>

#define FILTER_TAG "BLE_SCAN_FILTER"
#define CACHE_SIZE 20

typedef struct {
  uint8_t addr[6];
  int64_t last_seen;
} scan_cache_entry_t;

static scan_cache_entry_t s_scan_cache[CACHE_SIZE];
static int s_cache_head = 0;

static bool is_duplicate(const uint8_t *addr) {
  for (int i = 0; i < CACHE_SIZE; i++) {
    if (memcmp(s_scan_cache[i].addr, addr, 6) == 0) {
      return true;
    }
  }
  return false;
}

static void add_to_cache(const uint8_t *addr) {
  memcpy(s_scan_cache[s_cache_head].addr, addr, 6);
  s_cache_head = (s_cache_head + 1) % CACHE_SIZE;
}

static bool filter_by_name(const ble_adv_record_t *record) {
  if (!record->has_name)
    return false;
  // Check if recorded name contains the target string
  if (strstr(record->device_name, BLE_FILTER_NAME_STR) != NULL) {
    return true;
  }
  return false;
}

bool ble_filter_process(const ble_adv_record_t *record) {
  // 1. Software Deduplication implementation (if controller filter misses or
  // isn't used)
  if (is_duplicate(record->addr)) {
    return false;
  }

  // 2. Add to cache to prevent re-processing immediately
  add_to_cache(record->addr);

  // 3. Logic Filters
  if (!filter_by_name(record)) {
    return false;
  }

  return true;
}

void ble_filter_clear_cache(void) {
  memset(s_scan_cache, 0, sizeof(s_scan_cache));
  s_cache_head = 0;
  ESP_LOGI(FILTER_TAG, "Scan cache cleared");
}
