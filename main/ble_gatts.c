/*
 * ble_gatts.c
 * Minimal GATT Server Implementation
 */

#include "ble_gatts.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

#define GATTS_TAG "BLE_GATTS"

#define GATTS_SERVICE_UUID_TEST 0x00FF
#define GATTS_CHAR_UUID_TEST 0xFF01
#define GATTS_NUM_HANDLE_TEST 4

#define PROFILE_NUM 1
#define PROFILE_APP_ID 0

static void gatts_profile_a_event_handler(esp_gatts_cb_event_t event,
                                          esp_gatt_if_t gatts_if,
                                          esp_ble_gatts_cb_param_t *param);

static struct gatts_profile_inst {
  esp_gatts_cb_t gatts_cb;
  uint16_t gatts_if;
  uint16_t app_id;
  uint16_t conn_id;
  uint16_t service_handle;
  esp_gatt_srvc_id_t service_id;
  uint16_t char_handle;
  esp_bt_uuid_t char_uuid;
  esp_gatt_perm_t perm;
  esp_gatt_char_prop_t property;
  uint16_t descr_handle;
  esp_bt_uuid_t descr_uuid;
} gl_profile_tab[PROFILE_NUM] = {
    [PROFILE_APP_ID] =
        {
            .gatts_cb = gatts_profile_a_event_handler,
            .gatts_if = ESP_GATT_IF_NONE,
        },
};

static void gatts_profile_a_event_handler(esp_gatts_cb_event_t event,
                                          esp_gatt_if_t gatts_if,
                                          esp_ble_gatts_cb_param_t *param) {
  switch (event) {
  case ESP_GATTS_REG_EVT:
    ESP_LOGI(GATTS_TAG, "REGISTER_APP_EVT, status %d, app_id %d",
             param->reg.status, param->reg.app_id);
    gl_profile_tab[PROFILE_APP_ID].service_id.is_primary = true;
    gl_profile_tab[PROFILE_APP_ID].service_id.id.inst_id = 0x00;
    gl_profile_tab[PROFILE_APP_ID].service_id.id.uuid.len = ESP_UUID_LEN_16;
    gl_profile_tab[PROFILE_APP_ID].service_id.id.uuid.uuid.uuid16 =
        GATTS_SERVICE_UUID_TEST;

    esp_ble_gatts_create_service(gatts_if,
                                 &gl_profile_tab[PROFILE_APP_ID].service_id,
                                 GATTS_NUM_HANDLE_TEST);
    break;
  case ESP_GATTS_READ_EVT:
    ESP_LOGI(GATTS_TAG,
             "GATT_READ_EVT, conn_id %d, trans_id %" PRIu32 ", handle %d",
             param->read.conn_id, param->read.trans_id, param->read.handle);
    break;
  case ESP_GATTS_WRITE_EVT:
    ESP_LOGI(GATTS_TAG,
             "GATT_WRITE_EVT, conn_id %d, trans_id %" PRIu32 ", handle %d",
             param->write.conn_id, param->write.trans_id, param->write.handle);
    esp_ble_gatts_send_response(gatts_if, param->write.conn_id,
                                param->write.trans_id, ESP_GATT_OK, NULL);
    break;
  case ESP_GATTS_EXEC_WRITE_EVT:
  case ESP_GATTS_MTU_EVT:
  case ESP_GATTS_CONF_EVT:
  case ESP_GATTS_UNREG_EVT:
  case ESP_GATTS_CREATE_EVT:
    ESP_LOGI(GATTS_TAG, "CREATE_SERVICE_EVT, status %d,  service_handle %d",
             param->create.status, param->create.service_handle);
    gl_profile_tab[PROFILE_APP_ID].service_handle =
        param->create.service_handle;
    gl_profile_tab[PROFILE_APP_ID].char_uuid.len = ESP_UUID_LEN_16;
    gl_profile_tab[PROFILE_APP_ID].char_uuid.uuid.uuid16 = GATTS_CHAR_UUID_TEST;

    esp_ble_gatts_start_service(gl_profile_tab[PROFILE_APP_ID].service_handle);

    esp_ble_gatts_add_char(
        gl_profile_tab[PROFILE_APP_ID].service_handle,
        &gl_profile_tab[PROFILE_APP_ID].char_uuid,
        ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
        ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE, NULL, NULL);
    break;
  case ESP_GATTS_ADD_INCL_SRVC_EVT:
  case ESP_GATTS_ADD_CHAR_EVT:
    ESP_LOGI(GATTS_TAG,
             "ADD_CHAR_EVT, status %d,  attr_handle %d, service_handle %d",
             param->add_char.status, param->add_char.attr_handle,
             param->add_char.service_handle);
    gl_profile_tab[PROFILE_APP_ID].char_handle = param->add_char.attr_handle;
    break;
  case ESP_GATTS_ADD_CHAR_DESCR_EVT:
  case ESP_GATTS_DELETE_EVT:
  case ESP_GATTS_START_EVT:
    ESP_LOGI(GATTS_TAG, "SERVICE_START_EVT, status %d, service_handle %d",
             param->start.status, param->start.service_handle);
    break;
  case ESP_GATTS_STOP_EVT:
  case ESP_GATTS_CONNECT_EVT:
    ESP_LOGI(GATTS_TAG,
             "CONNECT_EVT, conn_id %d, remote %02x:%02x:%02x:%02x:%02x:%02x",
             param->connect.conn_id, param->connect.remote_bda[0],
             param->connect.remote_bda[1], param->connect.remote_bda[2],
             param->connect.remote_bda[3], param->connect.remote_bda[4],
             param->connect.remote_bda[5]);
    gl_profile_tab[PROFILE_APP_ID].conn_id = param->connect.conn_id;
    break;
  case ESP_GATTS_DISCONNECT_EVT:
    ESP_LOGI(GATTS_TAG, "DISCONNECT_EVT, disconnect reason 0x%x",
             param->disconnect.reason);
    // Advertising should be restarted by the GAP event handler or adv module if
    // needed But in our case extended advertising is separate logic.
    break;
  case ESP_GATTS_OPEN_EVT:
  case ESP_GATTS_CANCEL_OPEN_EVT:
  case ESP_GATTS_CLOSE_EVT:
  case ESP_GATTS_LISTEN_EVT:
  case ESP_GATTS_CONGEST_EVT:
  default:
    break;
  }
}

static void gatts_event_handler(esp_gatts_cb_event_t event,
                                esp_gatt_if_t gatts_if,
                                esp_ble_gatts_cb_param_t *param) {
  if (event == ESP_GATTS_REG_EVT) {
    if (param->reg.status == ESP_GATT_OK) {
      gl_profile_tab[param->reg.app_id].gatts_if = gatts_if;
    } else {
      ESP_LOGI(GATTS_TAG, "Reg app failed, app_id %04x, status %d",
               param->reg.app_id, param->reg.status);
      return;
    }
  }

  /* If the gatts_if equal to profile A, call profile A cb handler,
   * so here call each profile's callback */
  do {
    int idx;
    for (idx = 0; idx < PROFILE_NUM; idx++) {
      if (gatts_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, not specify a
                                             certain gatt_if, need to call every
                                             profile cb function */
          gatts_if == gl_profile_tab[idx].gatts_if) {
        if (gl_profile_tab[idx].gatts_cb) {
          gl_profile_tab[idx].gatts_cb(event, gatts_if, param);
        }
      }
    }
  } while (0);
}

void ble_gatts_init(void) {
  esp_err_t ret = esp_ble_gatts_register_callback(gatts_event_handler);
  if (ret) {
    ESP_LOGE(GATTS_TAG, "gatts register error, error code = %x", ret);
    return;
  }
  ret = esp_ble_gatts_app_register(PROFILE_APP_ID);
  if (ret) {
    ESP_LOGE(GATTS_TAG, "gatts app register error, error code = %x", ret);
    return;
  }
}
