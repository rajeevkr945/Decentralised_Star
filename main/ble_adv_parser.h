#ifndef BLE_ADV_PARSER_H
#define BLE_ADV_PARSER_H

#include "ble_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Parse raw advertisement payload into structured record
 *
 * @param payload Raw payload bytes
 * @param len Length of payload
 * @param out_record Pointer to output record structure (must be allocated by
 * caller)
 */
void ble_parse_adv_data(const uint8_t *payload, uint8_t len,
                        ble_adv_record_t *out_record);

#ifdef __cplusplus
}
#endif

#endif // BLE_ADV_PARSER_H
