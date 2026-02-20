#ifndef BLE_SCAN_H
#define BLE_SCAN_H

#include "esp_gap_ble_api.h"

#ifdef __cplusplus
extern "C" {
#endif

void ble_scan_init(void);
void ble_scan_process_result(esp_ble_gap_cb_param_t *param);
void ble_scan_on_stop_complete(void);

#ifdef __cplusplus
}
#endif

#endif // BLE_SCAN_H
