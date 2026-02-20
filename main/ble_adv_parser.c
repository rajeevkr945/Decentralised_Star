/*
 * ble_adv_parser.c
 * Advertisement Data Parser Implementation
 */

#include "ble_adv_parser.h"
#include "ble_common.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>

#define PARSER_TAG "BLE_ADV_PARSER"

// AD Types (Bluetooth SIG Assigned Numbers)
#define AD_TYPE_FLAGS 0x01
#define AD_TYPE_NAME_SHORT 0x08
#define AD_TYPE_NAME_CMPL 0x09
#define AD_TYPE_MANU 0xFF

void ble_parse_adv_data(const uint8_t *payload, uint8_t len,
                        ble_adv_record_t *out_record) {
  if (!payload || !out_record)
    return;

  // Reset optional fields
  out_record->has_name = false;
  out_record->has_mfg_data = false;
  out_record->mfg_data_len = 0;
  out_record->device_name[0] = '\0';

  uint8_t index = 0;
  while (index < len) {
    uint8_t length = payload[index];
    if (length == 0)
      break; // End of data

    uint8_t type = payload[index + 1];
    uint8_t *data = (uint8_t *)&payload[index + 2];
    uint8_t data_len = length - 1;

    if (index + length + 1 > len)
      break; // Malformed

    switch (type) {
    case AD_TYPE_NAME_CMPL:
    case AD_TYPE_NAME_SHORT:
      if (!out_record->has_name) {
        uint8_t copy_len = (data_len < sizeof(out_record->device_name) - 1)
                               ? data_len
                               : (sizeof(out_record->device_name) - 1);
        memcpy(out_record->device_name, data, copy_len);
        out_record->device_name[copy_len] = '\0';
        out_record->has_name = true;
      }
      break;

    case AD_TYPE_MANU:
      if (!out_record->has_mfg_data) {
        uint8_t copy_len = (data_len < sizeof(out_record->mfg_data))
                               ? data_len
                               : sizeof(out_record->mfg_data);
        memcpy(out_record->mfg_data, data, copy_len);
        out_record->mfg_data_len = copy_len;
        out_record->has_mfg_data = true;
      }
      break;

    case AD_TYPE_FLAGS:
      if (data_len > 0) {
        out_record->flags = data[0];
      }
      break;

    default:
      break;
    }

    index += length + 1;
  }
}
