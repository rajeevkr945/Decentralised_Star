#ifndef BLE_SCAN_FILTER_H
#define BLE_SCAN_FILTER_H

#include "ble_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Process a scan record through the filter chain
 * @return true if record passes all filters, false otherwise
 */
bool ble_filter_process(const ble_adv_record_t *record);

/**
 * @brief Clear the software deduplication cache
 */
void ble_filter_clear_cache(void);

#ifdef __cplusplus
}
#endif

#endif // BLE_SCAN_FILTER_H
