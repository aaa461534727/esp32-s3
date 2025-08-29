// #include "ble_peripheral.h"


// // #define LOG_TAG "GATTS"

// // ///Declare the static function
// // static void gatts_profile_a_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
// // static void gatts_profile_b_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

// // #define GATTS_SERVICE_UUID_TEST_A   0x00FF
// // #define GATTS_CHAR_UUID_TEST_A      0xFF01
// // #define GATTS_DESCR_UUID_TEST_A     0x3333
// // #define GATTS_NUM_HANDLE_TEST_A     4

// // #define GATTS_SERVICE_UUID_TEST_B   0x00EE
// // #define GATTS_CHAR_UUID_TEST_B      0xEE01
// // #define GATTS_DESCR_UUID_TEST_B     0x2222
// // #define GATTS_NUM_HANDLE_TEST_B     4

// // #define TEST_DEVICE_NAME            "ESP_BLE"
// // #define TEST_MANUFACTURER_DATA_LEN  17

// // #define GATTS_DEMO_CHAR_VAL_LEN_MAX 0x40

// // #define PREPARE_BUF_MAX_SIZE 1024

// // static uint8_t char1_str[] = {0x11,0x22,0x33};
// // static esp_gatt_char_prop_t a_property = 0;
// // static esp_gatt_char_prop_t b_property = 0;

// // static esp_attr_value_t gatts_demo_char1_val =
// // {
// //     .attr_max_len = GATTS_DEMO_CHAR_VAL_LEN_MAX,
// //     .attr_len     = sizeof(char1_str),
// //     .attr_value   = char1_str,
// // };

// // static uint8_t adv_config_done = 0;
// // #define adv_config_flag      (1 << 0)
// // #define scan_rsp_config_flag (1 << 1)

// // #ifdef CONFIG_SET_RAW_ADV_DATA
// // #else

// // static uint8_t adv_service_uuid128[32] = {
// //     /* LSB <--------------------------------------------------------------------------------> MSB */
// //     //first uuid, 16bit, [12],[13] is the value
// //     0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xEE, 0x00, 0x00, 0x00,
// //     //second uuid, 32bit, [12], [13], [14], [15] is the value
// //     0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
// // };

// // // The length of adv data must be less than 31 bytes
// // //static uint8_t test_manufacturer[TEST_MANUFACTURER_DATA_LEN] =  {0x12, 0x23, 0x45, 0x56};
// // //adv data
// // static esp_ble_adv_data_t adv_data = {
// //     .set_scan_rsp = false,
// //     .include_name = false,
// //     .include_txpower = false,
// //     .min_interval = 0x0006, //slave connection min interval, Time = min_interval * 1.25 msec
// //     .max_interval = 0x0010, //slave connection max interval, Time = max_interval * 1.25 msec
// //     .appearance = 0x00,
// //     .manufacturer_len = 0, //TEST_MANUFACTURER_DATA_LEN,
// //     .p_manufacturer_data =  NULL, //&test_manufacturer[0],
// //     .service_data_len = 0,
// //     .p_service_data = NULL,
// //     .service_uuid_len = sizeof(adv_service_uuid128),
// //     .p_service_uuid = adv_service_uuid128,
// //     .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
// // };

// // // scan response data
// // static esp_ble_adv_data_t scan_rsp_data = {
// //     .set_scan_rsp = true,
// //     .include_name = true,
// //     .include_txpower = true,
// //     .min_interval = 0x0006,
// //     .max_interval = 0x0010,
// //     .appearance = 0x00,
// //     .manufacturer_len = 0, //TEST_MANUFACTURER_DATA_LEN,
// //     .p_manufacturer_data =  NULL, //&test_manufacturer[0],
// //     .service_data_len = 0,
// //     .p_service_data = NULL,
// //     .service_uuid_len = sizeof(adv_service_uuid128),
// //     .p_service_uuid = adv_service_uuid128,
// //     .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
// // };

// // #endif /* CONFIG_SET_RAW_ADV_DATA */

// // static esp_ble_adv_params_t legacy_adv_params = {
// //     .adv_int_min        = 0x20,
// //     .adv_int_max        = 0x40,
// //     .adv_type           = ADV_TYPE_NONCONN_IND, //不可连接广播
// //     .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
// //     //.peer_addr            =
// //     //.peer_addr_type       =
// //     .channel_map        = ADV_CHNL_ALL,
// //     .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
// // };

// // #define PROFILE_NUM 2
// // #define PROFILE_A_APP_ID 0
// // #define PROFILE_B_APP_ID 1

// // struct gatts_profile_inst {
// //     esp_gatts_cb_t gatts_cb;
// //     uint16_t gatts_if;
// //     uint16_t app_id;
// //     uint16_t conn_id;
// //     uint16_t service_handle;
// //     esp_gatt_srvc_id_t service_id;
// //     uint16_t char_handle;
// //     esp_bt_uuid_t char_uuid;
// //     esp_gatt_perm_t perm;
// //     esp_gatt_char_prop_t property;
// //     uint16_t descr_handle;
// //     esp_bt_uuid_t descr_uuid;
// // };

// // /* One gatt-based profile one app_id and one gatts_if, this array will store the gatts_if returned by ESP_GATTS_REG_EVT */
// // static struct gatts_profile_inst gl_profile_tab[PROFILE_NUM] = {
// //     [PROFILE_A_APP_ID] = {
// //         .gatts_cb = gatts_profile_a_event_handler,
// //         .gatts_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
// //     },
// //     [PROFILE_B_APP_ID] = {
// //         .gatts_cb = gatts_profile_b_event_handler,                   /* This demo does not implement, similar as profile A */
// //         .gatts_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
// //     },
// // };

// // typedef struct {
// //     uint8_t                 *prepare_buf;
// //     int                     prepare_len;
// // } prepare_type_env_t;

// // static prepare_type_env_t a_prepare_write_env;
// // static prepare_type_env_t b_prepare_write_env;

// // void example_write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param);
// // void example_exec_write_event_env(prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param);

// // static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
// // {
// //     switch (event) {
// // #ifdef CONFIG_SET_RAW_ADV_DATA
// //     case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
// //         adv_config_done &= (~adv_config_flag);
// //         if (adv_config_done==0){
// //             esp_ble_gap_start_advertising(&legacy_adv_params);
// //         }
// //         break;
// //     case ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT:
// //         adv_config_done &= (~scan_rsp_config_flag);
// //         if (adv_config_done==0){
// //             esp_ble_gap_start_advertising(&legacy_adv_params);
// //         }
// //         break;
// // #else
// //     case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
// //         adv_config_done &= (~adv_config_flag);
// //         if (adv_config_done == 0){
// //             esp_ble_gap_start_advertising(&legacy_adv_params);
// //         }
// //         break;
// //     case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
// //         adv_config_done &= (~scan_rsp_config_flag);
// //         if (adv_config_done == 0){
// //             esp_ble_gap_start_advertising(&legacy_adv_params);
// //         }
// //         break;
// // #endif
// //     case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
// //         //advertising start complete event to indicate advertising start successfully or failed
// //         if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
// //             ESP_LOGE(LOG_TAG, "Advertising start failed");
// //         }
// //         break;
// //     case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
// //         if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
// //             ESP_LOGE(LOG_TAG, "Advertising stop failed");
// //         } else {
// //             ESP_LOGI(LOG_TAG, "Stop adv successfully");
// //         }
// //         break;
// //     case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
// //          ESP_LOGI(LOG_TAG, "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
// //                   param->update_conn_params.status,
// //                   param->update_conn_params.min_int,
// //                   param->update_conn_params.max_int,
// //                   param->update_conn_params.conn_int,
// //                   param->update_conn_params.latency,
// //                   param->update_conn_params.timeout);
// //         break;
// //     case ESP_GAP_BLE_SET_PKT_LENGTH_COMPLETE_EVT:
// //         ESP_LOGI(LOG_TAG, "packet length updated: rx = %d, tx = %d, status = %d",
// //                   param->pkt_data_length_cmpl.params.rx_len,
// //                   param->pkt_data_length_cmpl.params.tx_len,
// //                   param->pkt_data_length_cmpl.status);
// //         break;
// //     default:
// //         break;
// //     }
// // }

// // void example_write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param){
// //     esp_gatt_status_t status = ESP_GATT_OK;
// //     if (param->write.need_rsp){
// //         if (param->write.is_prep) {
// //             if (param->write.offset > PREPARE_BUF_MAX_SIZE) {
// //                 status = ESP_GATT_INVALID_OFFSET;
// //             } else if ((param->write.offset + param->write.len) > PREPARE_BUF_MAX_SIZE) {
// //                 status = ESP_GATT_INVALID_ATTR_LEN;
// //             }
// //             if (status == ESP_GATT_OK && prepare_write_env->prepare_buf == NULL) {
// //                 prepare_write_env->prepare_buf = (uint8_t *)malloc(PREPARE_BUF_MAX_SIZE*sizeof(uint8_t));
// //                 prepare_write_env->prepare_len = 0;
// //                 if (prepare_write_env->prepare_buf == NULL) {
// //                     ESP_LOGE(LOG_TAG, "Gatt_server prep no mem");
// //                     status = ESP_GATT_NO_RESOURCES;
// //                 }
// //             }

// //             esp_gatt_rsp_t *gatt_rsp = (esp_gatt_rsp_t *)malloc(sizeof(esp_gatt_rsp_t));
// //             if (gatt_rsp) {
// //                 gatt_rsp->attr_value.len = param->write.len;
// //                 gatt_rsp->attr_value.handle = param->write.handle;
// //                 gatt_rsp->attr_value.offset = param->write.offset;
// //                 gatt_rsp->attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
// //                 memcpy(gatt_rsp->attr_value.value, param->write.value, param->write.len);
// //                 esp_err_t response_err = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, gatt_rsp);
// //                 if (response_err != ESP_OK){
// //                     ESP_LOGE(LOG_TAG, "Send response error\n");
// //                 }
// //                 free(gatt_rsp);
// //             } else {
// //                 ESP_LOGE(LOG_TAG, "malloc failed, no resource to send response error\n");
// //                 status = ESP_GATT_NO_RESOURCES;
// //             }
// //             if (status != ESP_GATT_OK){
// //                 return;
// //             }
// //             memcpy(prepare_write_env->prepare_buf + param->write.offset,
// //                    param->write.value,
// //                    param->write.len);
// //             prepare_write_env->prepare_len += param->write.len;

// //         }else{
// //             esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, NULL);
// //         }
// //     }
// // }

// // void example_exec_write_event_env(prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param){
// //     if (param->exec_write.exec_write_flag == ESP_GATT_PREP_WRITE_EXEC){
// //         esp_log_buffer_hex(LOG_TAG, prepare_write_env->prepare_buf, prepare_write_env->prepare_len);
// //     }else{
// //         ESP_LOGI(LOG_TAG,"ESP_GATT_PREP_WRITE_CANCEL");
// //     }
// //     if (prepare_write_env->prepare_buf) {
// //         free(prepare_write_env->prepare_buf);
// //         prepare_write_env->prepare_buf = NULL;
// //     }
// //     prepare_write_env->prepare_len = 0;
// // }

// // static void gatts_profile_a_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
// //     switch (event) {
// //     case ESP_GATTS_REG_EVT:
// //         ESP_LOGI(LOG_TAG, "REGISTER_APP_EVT, status %d, app_id %d", param->reg.status, param->reg.app_id);
// //         gl_profile_tab[PROFILE_A_APP_ID].service_id.is_primary = true;
// //         gl_profile_tab[PROFILE_A_APP_ID].service_id.id.inst_id = 0x00;
// //         gl_profile_tab[PROFILE_A_APP_ID].service_id.id.uuid.len = ESP_UUID_LEN_16;
// //         gl_profile_tab[PROFILE_A_APP_ID].service_id.id.uuid.uuid.uuid16 = GATTS_SERVICE_UUID_TEST_A;

// //         esp_err_t set_dev_name_ret = esp_ble_gap_set_device_name(TEST_DEVICE_NAME);
// //         if (set_dev_name_ret){
// //             ESP_LOGE(LOG_TAG, "set device name failed, error code = %x", set_dev_name_ret);
// //         }
// // #ifdef CONFIG_SET_RAW_ADV_DATA
// //         esp_err_t raw_adv_ret = esp_ble_gap_config_adv_data_raw(raw_adv_data, sizeof(raw_adv_data));
// //         if (raw_adv_ret){
// //             ESP_LOGE(LOG_TAG, "config raw adv data failed, error code = %x ", raw_adv_ret);
// //         }
// //         adv_config_done |= adv_config_flag;
// //         esp_err_t raw_scan_ret = esp_ble_gap_config_scan_rsp_data_raw(raw_scan_rsp_data, sizeof(raw_scan_rsp_data));
// //         if (raw_scan_ret){
// //             ESP_LOGE(LOG_TAG, "config raw scan rsp data failed, error code = %x", raw_scan_ret);
// //         }
// //         adv_config_done |= scan_rsp_config_flag;
// // #else
// //         //config adv data
// //         esp_err_t ret = esp_ble_gap_config_adv_data(&adv_data);
// //         if (ret){
// //             ESP_LOGE(LOG_TAG, "config adv data failed, error code = %x", ret);
// //         }
// //         adv_config_done |= adv_config_flag;
// //         //config scan response data
// //         ret = esp_ble_gap_config_adv_data(&scan_rsp_data);
// //         if (ret){
// //             ESP_LOGE(LOG_TAG, "config scan response data failed, error code = %x", ret);
// //         }
// //         adv_config_done |= scan_rsp_config_flag;

// // #endif
// //         esp_ble_gatts_create_service(gatts_if, &gl_profile_tab[PROFILE_A_APP_ID].service_id, GATTS_NUM_HANDLE_TEST_A);
// //         break;
// //     case ESP_GATTS_READ_EVT: {
// //         ESP_LOGI(LOG_TAG, "GATT_READ_EVT, conn_id %d, trans_id %" PRIu32 ", handle %d", param->read.conn_id, param->read.trans_id, param->read.handle);
// //         esp_gatt_rsp_t rsp;
// //         memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
// //         rsp.attr_value.handle = param->read.handle;
// //         rsp.attr_value.len = 4;
// //         rsp.attr_value.value[0] = 0xde;
// //         rsp.attr_value.value[1] = 0xed;
// //         rsp.attr_value.value[2] = 0xbe;
// //         rsp.attr_value.value[3] = 0xef;
// //         esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id,
// //                                     ESP_GATT_OK, &rsp);
// //         break;
// //     }
// //     case ESP_GATTS_WRITE_EVT: {
// //         ESP_LOGI(LOG_TAG, "GATT_WRITE_EVT, conn_id %d, trans_id %" PRIu32 ", handle %d", param->write.conn_id, param->write.trans_id, param->write.handle);
// //         if (!param->write.is_prep){
// //             ESP_LOGI(LOG_TAG, "GATT_WRITE_EVT, value len %d, value :", param->write.len);
// //             esp_log_buffer_hex(LOG_TAG, param->write.value, param->write.len);
// //             if (gl_profile_tab[PROFILE_A_APP_ID].descr_handle == param->write.handle && param->write.len == 2){
// //                 uint16_t descr_value = param->write.value[1]<<8 | param->write.value[0];
// //                 if (descr_value == 0x0001){
// //                     if (a_property & ESP_GATT_CHAR_PROP_BIT_NOTIFY){
// //                         ESP_LOGI(LOG_TAG, "notify enable");
// //                         uint8_t notify_data[15];
// //                         for (int i = 0; i < sizeof(notify_data); ++i)
// //                         {
// //                             notify_data[i] = i%0xff;
// //                         }
// //                         //the size of notify_data[] need less than MTU size
// //                         esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, gl_profile_tab[PROFILE_A_APP_ID].char_handle,
// //                                                 sizeof(notify_data), notify_data, false);
// //                     }
// //                 }else if (descr_value == 0x0002){
// //                     if (a_property & ESP_GATT_CHAR_PROP_BIT_INDICATE){
// //                         ESP_LOGI(LOG_TAG, "indicate enable");
// //                         uint8_t indicate_data[15];
// //                         for (int i = 0; i < sizeof(indicate_data); ++i)
// //                         {
// //                             indicate_data[i] = i%0xff;
// //                         }
// //                         //the size of indicate_data[] need less than MTU size
// //                         esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, gl_profile_tab[PROFILE_A_APP_ID].char_handle,
// //                                                 sizeof(indicate_data), indicate_data, true);
// //                     }
// //                 }
// //                 else if (descr_value == 0x0000){
// //                     ESP_LOGI(LOG_TAG, "notify/indicate disable ");
// //                 }else{
// //                     ESP_LOGE(LOG_TAG, "unknown descr value");
// //                     esp_log_buffer_hex(LOG_TAG, param->write.value, param->write.len);
// //                 }

// //             }
// //         }
// //         example_write_event_env(gatts_if, &a_prepare_write_env, param);
// //         break;
// //     }
// //     case ESP_GATTS_EXEC_WRITE_EVT:
// //         ESP_LOGI(LOG_TAG,"ESP_GATTS_EXEC_WRITE_EVT");
// //         esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
// //         example_exec_write_event_env(&a_prepare_write_env, param);
// //         break;
// //     case ESP_GATTS_MTU_EVT:
// //         ESP_LOGI(LOG_TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
// //         break;
// //     case ESP_GATTS_UNREG_EVT:
// //         break;
// //     case ESP_GATTS_CREATE_EVT:
// //         ESP_LOGI(LOG_TAG, "CREATE_SERVICE_EVT, status %d,  service_handle %d", param->create.status, param->create.service_handle);
// //         gl_profile_tab[PROFILE_A_APP_ID].service_handle = param->create.service_handle;
// //         gl_profile_tab[PROFILE_A_APP_ID].char_uuid.len = ESP_UUID_LEN_16;
// //         gl_profile_tab[PROFILE_A_APP_ID].char_uuid.uuid.uuid16 = GATTS_CHAR_UUID_TEST_A;

// //         esp_ble_gatts_start_service(gl_profile_tab[PROFILE_A_APP_ID].service_handle);
// //         a_property = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
// //         esp_err_t add_char_ret = esp_ble_gatts_add_char(gl_profile_tab[PROFILE_A_APP_ID].service_handle, &gl_profile_tab[PROFILE_A_APP_ID].char_uuid,
// //                                                         ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
// //                                                         a_property,
// //                                                         &gatts_demo_char1_val, NULL);
// //         if (add_char_ret){
// //             ESP_LOGE(LOG_TAG, "add char failed, error code =%x",add_char_ret);
// //         }
// //         break;
// //     case ESP_GATTS_ADD_INCL_SRVC_EVT:
// //         break;
// //     case ESP_GATTS_ADD_CHAR_EVT: {
// //         uint16_t length = 0;
// //         const uint8_t *prf_char;

// //         ESP_LOGI(LOG_TAG, "ADD_CHAR_EVT, status %d,  attr_handle %d, service_handle %d",
// //                 param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle);
// //         gl_profile_tab[PROFILE_A_APP_ID].char_handle = param->add_char.attr_handle;
// //         gl_profile_tab[PROFILE_A_APP_ID].descr_uuid.len = ESP_UUID_LEN_16;
// //         gl_profile_tab[PROFILE_A_APP_ID].descr_uuid.uuid.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
// //         esp_err_t get_attr_ret = esp_ble_gatts_get_attr_value(param->add_char.attr_handle,  &length, &prf_char);
// //         if (get_attr_ret == ESP_FAIL){
// //             ESP_LOGE(LOG_TAG, "ILLEGAL HANDLE");
// //         }

// //         ESP_LOGI(LOG_TAG, "the gatts demo char length = %x", length);
// //         for(int i = 0; i < length; i++){
// //             ESP_LOGI(LOG_TAG, "prf_char[%x] =%x",i,prf_char[i]);
// //         }
// //         esp_err_t add_descr_ret = esp_ble_gatts_add_char_descr(gl_profile_tab[PROFILE_A_APP_ID].service_handle, &gl_profile_tab[PROFILE_A_APP_ID].descr_uuid,
// //                                                                 ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, NULL, NULL);
// //         if (add_descr_ret){
// //             ESP_LOGE(LOG_TAG, "add char descr failed, error code =%x", add_descr_ret);
// //         }
// //         break;
// //     }
// //     case ESP_GATTS_ADD_CHAR_DESCR_EVT:
// //         gl_profile_tab[PROFILE_A_APP_ID].descr_handle = param->add_char_descr.attr_handle;
// //         ESP_LOGI(LOG_TAG, "ADD_DESCR_EVT, status %d, attr_handle %d, service_handle %d",
// //                  param->add_char_descr.status, param->add_char_descr.attr_handle, param->add_char_descr.service_handle);
// //         break;
// //     case ESP_GATTS_DELETE_EVT:
// //         break;
// //     case ESP_GATTS_START_EVT:
// //         ESP_LOGI(LOG_TAG, "SERVICE_START_EVT, status %d, service_handle %d",
// //                  param->start.status, param->start.service_handle);
// //         break;
// //     case ESP_GATTS_STOP_EVT:
// //         break;
// //     case ESP_GATTS_CONNECT_EVT: {
// //         esp_ble_conn_update_params_t conn_params = {0};
// //         memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
// //         /* For the IOS system, please reference the apple official documents about the ble connection parameters restrictions. */
// //         conn_params.latency = 0;
// //         conn_params.max_int = 0x20;    // max_int = 0x20*1.25ms = 40ms
// //         conn_params.min_int = 0x10;    // min_int = 0x10*1.25ms = 20ms
// //         conn_params.timeout = 400;    // timeout = 400*10ms = 4000ms
// //         ESP_LOGI(LOG_TAG, "ESP_GATTS_CONNECT_EVT, conn_id %d, remote %02x:%02x:%02x:%02x:%02x:%02x:",
// //                  param->connect.conn_id,
// //                  param->connect.remote_bda[0], param->connect.remote_bda[1], param->connect.remote_bda[2],
// //                  param->connect.remote_bda[3], param->connect.remote_bda[4], param->connect.remote_bda[5]);
// //         gl_profile_tab[PROFILE_A_APP_ID].conn_id = param->connect.conn_id;
// //         //start sent the update connection parameters to the peer device.
// //         esp_ble_gap_update_conn_params(&conn_params);
// //         break;
// //     }
// //     case ESP_GATTS_DISCONNECT_EVT:
// //         ESP_LOGI(LOG_TAG, "ESP_GATTS_DISCONNECT_EVT, disconnect reason 0x%x", param->disconnect.reason);
// //         esp_ble_gap_start_advertising(&legacy_adv_params);
// //         break;
// //     case ESP_GATTS_CONF_EVT:
// //         ESP_LOGI(LOG_TAG, "ESP_GATTS_CONF_EVT, status %d attr_handle %d", param->conf.status, param->conf.handle);
// //         if (param->conf.status != ESP_GATT_OK){
// //             esp_log_buffer_hex(LOG_TAG, param->conf.value, param->conf.len);
// //         }
// //         break;
// //     case ESP_GATTS_OPEN_EVT:
// //     case ESP_GATTS_CANCEL_OPEN_EVT:
// //     case ESP_GATTS_CLOSE_EVT:
// //     case ESP_GATTS_LISTEN_EVT:
// //     case ESP_GATTS_CONGEST_EVT:
// //     default:
// //         break;
// //     }
// // }

// // static void gatts_profile_b_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
// //     switch (event) {
// //     case ESP_GATTS_REG_EVT:
// //         ESP_LOGI(LOG_TAG, "REGISTER_APP_EVT, status %d, app_id %d", param->reg.status, param->reg.app_id);
// //         gl_profile_tab[PROFILE_B_APP_ID].service_id.is_primary = true;
// //         gl_profile_tab[PROFILE_B_APP_ID].service_id.id.inst_id = 0x00;
// //         gl_profile_tab[PROFILE_B_APP_ID].service_id.id.uuid.len = ESP_UUID_LEN_16;
// //         gl_profile_tab[PROFILE_B_APP_ID].service_id.id.uuid.uuid.uuid16 = GATTS_SERVICE_UUID_TEST_B;

// //         esp_ble_gatts_create_service(gatts_if, &gl_profile_tab[PROFILE_B_APP_ID].service_id, GATTS_NUM_HANDLE_TEST_B);
// //         break;
// //     case ESP_GATTS_READ_EVT: {
// //         ESP_LOGI(LOG_TAG, "GATT_READ_EVT, conn_id %d, trans_id %" PRIu32 ", handle %d", param->read.conn_id, param->read.trans_id, param->read.handle);
// //         esp_gatt_rsp_t rsp;
// //         memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
// //         rsp.attr_value.handle = param->read.handle;
// //         rsp.attr_value.len = 4;
// //         rsp.attr_value.value[0] = 0xde;
// //         rsp.attr_value.value[1] = 0xed;
// //         rsp.attr_value.value[2] = 0xbe;
// //         rsp.attr_value.value[3] = 0xef;
// //         esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id,
// //                                     ESP_GATT_OK, &rsp);
// //         break;
// //     }
// //     case ESP_GATTS_WRITE_EVT: {
// //         ESP_LOGI(LOG_TAG, "GATT_WRITE_EVT, conn_id %d, trans_id %" PRIu32 ", handle %d", param->write.conn_id, param->write.trans_id, param->write.handle);
// //         if (!param->write.is_prep){
// //             ESP_LOGI(LOG_TAG, "GATT_WRITE_EVT, value len %d, value :", param->write.len);
// //             esp_log_buffer_hex(LOG_TAG, param->write.value, param->write.len);
// //             if (gl_profile_tab[PROFILE_B_APP_ID].descr_handle == param->write.handle && param->write.len == 2){
// //                 uint16_t descr_value= param->write.value[1]<<8 | param->write.value[0];
// //                 if (descr_value == 0x0001){
// //                     if (b_property & ESP_GATT_CHAR_PROP_BIT_NOTIFY){
// //                         ESP_LOGI(LOG_TAG, "notify enable");
// //                         uint8_t notify_data[15];
// //                         for (int i = 0; i < sizeof(notify_data); ++i)
// //                         {
// //                             notify_data[i] = i%0xff;
// //                         }
// //                         //the size of notify_data[] need less than MTU size
// //                         esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, gl_profile_tab[PROFILE_B_APP_ID].char_handle,
// //                                                 sizeof(notify_data), notify_data, false);
// //                     }
// //                 }else if (descr_value == 0x0002){
// //                     if (b_property & ESP_GATT_CHAR_PROP_BIT_INDICATE){
// //                         ESP_LOGI(LOG_TAG, "indicate enable");
// //                         uint8_t indicate_data[15];
// //                         for (int i = 0; i < sizeof(indicate_data); ++i)
// //                         {
// //                             indicate_data[i] = i%0xff;
// //                         }
// //                         //the size of indicate_data[] need less than MTU size
// //                         esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, gl_profile_tab[PROFILE_B_APP_ID].char_handle,
// //                                                 sizeof(indicate_data), indicate_data, true);
// //                     }
// //                 }
// //                 else if (descr_value == 0x0000){
// //                     ESP_LOGI(LOG_TAG, "notify/indicate disable ");
// //                 }else{
// //                     ESP_LOGE(LOG_TAG, "unknown value");
// //                 }

// //             }
// //         }
// //         example_write_event_env(gatts_if, &b_prepare_write_env, param);
// //         break;
// //     }
// //     case ESP_GATTS_EXEC_WRITE_EVT:
// //         ESP_LOGI(LOG_TAG,"ESP_GATTS_EXEC_WRITE_EVT");
// //         esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
// //         example_exec_write_event_env(&b_prepare_write_env, param);
// //         break;
// //     case ESP_GATTS_MTU_EVT:
// //         ESP_LOGI(LOG_TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
// //         break;
// //     case ESP_GATTS_UNREG_EVT:
// //         break;
// //     case ESP_GATTS_CREATE_EVT:
// //         ESP_LOGI(LOG_TAG, "CREATE_SERVICE_EVT, status %d,  service_handle %d", param->create.status, param->create.service_handle);
// //         gl_profile_tab[PROFILE_B_APP_ID].service_handle = param->create.service_handle;
// //         gl_profile_tab[PROFILE_B_APP_ID].char_uuid.len = ESP_UUID_LEN_16;
// //         gl_profile_tab[PROFILE_B_APP_ID].char_uuid.uuid.uuid16 = GATTS_CHAR_UUID_TEST_B;

// //         esp_ble_gatts_start_service(gl_profile_tab[PROFILE_B_APP_ID].service_handle);
// //         b_property = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
// //         esp_err_t add_char_ret =esp_ble_gatts_add_char( gl_profile_tab[PROFILE_B_APP_ID].service_handle, &gl_profile_tab[PROFILE_B_APP_ID].char_uuid,
// //                                                         ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
// //                                                         b_property,
// //                                                         NULL, NULL);
// //         if (add_char_ret){
// //             ESP_LOGE(LOG_TAG, "add char failed, error code =%x",add_char_ret);
// //         }
// //         break;
// //     case ESP_GATTS_ADD_INCL_SRVC_EVT:
// //         break;
// //     case ESP_GATTS_ADD_CHAR_EVT:
// //         ESP_LOGI(LOG_TAG, "ADD_CHAR_EVT, status %d,  attr_handle %d, service_handle %d",
// //                  param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle);

// //         gl_profile_tab[PROFILE_B_APP_ID].char_handle = param->add_char.attr_handle;
// //         gl_profile_tab[PROFILE_B_APP_ID].descr_uuid.len = ESP_UUID_LEN_16;
// //         gl_profile_tab[PROFILE_B_APP_ID].descr_uuid.uuid.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
// //         esp_ble_gatts_add_char_descr(gl_profile_tab[PROFILE_B_APP_ID].service_handle, &gl_profile_tab[PROFILE_B_APP_ID].descr_uuid,
// //                                      ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
// //                                      NULL, NULL);
// //         break;
// //     case ESP_GATTS_ADD_CHAR_DESCR_EVT:
// //         gl_profile_tab[PROFILE_B_APP_ID].descr_handle = param->add_char_descr.attr_handle;
// //         ESP_LOGI(LOG_TAG, "ADD_DESCR_EVT, status %d, attr_handle %d, service_handle %d",
// //                  param->add_char_descr.status, param->add_char_descr.attr_handle, param->add_char_descr.service_handle);
// //         break;
// //     case ESP_GATTS_DELETE_EVT:
// //         break;
// //     case ESP_GATTS_START_EVT:
// //         ESP_LOGI(LOG_TAG, "SERVICE_START_EVT, status %d, service_handle %d",
// //                  param->start.status, param->start.service_handle);
// //         break;
// //     case ESP_GATTS_STOP_EVT:
// //         break;
// //     case ESP_GATTS_CONNECT_EVT:
// //         ESP_LOGI(LOG_TAG, "CONNECT_EVT, conn_id %d, remote %02x:%02x:%02x:%02x:%02x:%02x:",
// //                  param->connect.conn_id,
// //                  param->connect.remote_bda[0], param->connect.remote_bda[1], param->connect.remote_bda[2],
// //                  param->connect.remote_bda[3], param->connect.remote_bda[4], param->connect.remote_bda[5]);
// //         gl_profile_tab[PROFILE_B_APP_ID].conn_id = param->connect.conn_id;
// //         break;
// //     case ESP_GATTS_CONF_EVT:
// //         ESP_LOGI(LOG_TAG, "ESP_GATTS_CONF_EVT status %d attr_handle %d", param->conf.status, param->conf.handle);
// //         if (param->conf.status != ESP_GATT_OK){
// //             esp_log_buffer_hex(LOG_TAG, param->conf.value, param->conf.len);
// //         }
// //     break;
// //     case ESP_GATTS_DISCONNECT_EVT:
// //     case ESP_GATTS_OPEN_EVT:
// //     case ESP_GATTS_CANCEL_OPEN_EVT:
// //     case ESP_GATTS_CLOSE_EVT:
// //     case ESP_GATTS_LISTEN_EVT:
// //     case ESP_GATTS_CONGEST_EVT:
// //     default:
// //         break;
// //     }
// // }

// // static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
// // {
// //     /* If event is register event, store the gatts_if for each profile */
// //     if (event == ESP_GATTS_REG_EVT) {
// //         if (param->reg.status == ESP_GATT_OK) {
// //             gl_profile_tab[param->reg.app_id].gatts_if = gatts_if;
// //         } else {
// //             ESP_LOGI(LOG_TAG, "Reg app failed, app_id %04x, status %d",
// //                     param->reg.app_id,
// //                     param->reg.status);
// //             return;
// //         }
// //     }

// //     /* If the gatts_if equal to profile A, call profile A cb handler,
// //      * so here call each profile's callback */
// //     do {
// //         int idx;
// //         for (idx = 0; idx < PROFILE_NUM; idx++) {
// //             if (gatts_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
// //                     gatts_if == gl_profile_tab[idx].gatts_if) {
// //                 if (gl_profile_tab[idx].gatts_cb) {
// //                     gl_profile_tab[idx].gatts_cb(event, gatts_if, param);
// //                 }
// //             }
// //         }
// //     } while (0);
// // }


// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "freertos/event_groups.h"
// #include "esp_system.h"
// #include "esp_log.h"
// #include "nvs_flash.h"
// #include "esp_bt.h"

// #include "esp_gap_ble_api.h"
// #include "esp_gatts_api.h"
// #include "esp_bt_defs.h"
// #include "esp_bt_main.h"
// #include "esp_gatt_common_api.h"

// #include "sdkconfig.h"

// #include "freertos/semphr.h"

// #define LOG_TAG "BLE ADV"


// static SemaphoreHandle_t set_sem = NULL;

// uint8_t addr_1m[6] = {0xc0, 0xde, 0x52, 0x00, 0x00, 0x01};
// uint8_t addr_2m[6] = {0xc0, 0xde, 0x52, 0x00, 0x00, 0x02};
// uint8_t addr_legacy[6] = {0xc0, 0xde, 0x52, 0x00, 0x00, 0x03};
// uint8_t addr_coded[6] = {0xc0, 0xde, 0x52, 0x00, 0x00, 0x04};

// esp_ble_gap_ext_adv_params_t ext_adv_params_1M = {
//     .type = ESP_BLE_GAP_SET_EXT_ADV_PROP_CONNECTABLE,
//     .interval_min = 0x30,
//     .interval_max = 0x30,
//     .channel_map = ADV_CHNL_ALL,
//     .filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
//     .primary_phy = ESP_BLE_GAP_PHY_1M,
//     .max_skip = 0,
//     .secondary_phy = ESP_BLE_GAP_PHY_1M,
//     .sid = 0,
//     .scan_req_notif = false,
//     .own_addr_type = BLE_ADDR_TYPE_RANDOM,
//     .tx_power = EXT_ADV_TX_PWR_NO_PREFERENCE,
// };

// esp_ble_gap_ext_adv_params_t ext_adv_params_2M = {
//     .type = ESP_BLE_GAP_SET_EXT_ADV_PROP_SCANNABLE,
//     .interval_min = 0x40,
//     .interval_max = 0x40,
//     .channel_map = ADV_CHNL_ALL,
//     .filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
//     .primary_phy = ESP_BLE_GAP_PHY_1M,
//     .max_skip = 0,
//     .secondary_phy = ESP_BLE_GAP_PHY_2M,
//     .sid = 1,
//     .scan_req_notif = false,
//     .own_addr_type = BLE_ADDR_TYPE_RANDOM,
//     .tx_power = EXT_ADV_TX_PWR_NO_PREFERENCE,
// };

// esp_ble_gap_ext_adv_params_t legacy_adv_params = {
//     .type = ESP_BLE_GAP_SET_EXT_ADV_PROP_LEGACY_IND,
//     .interval_min = 0x45,
//     .interval_max = 0x45,
//     .channel_map = ADV_CHNL_ALL,
//     .filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
//     .primary_phy = ESP_BLE_GAP_PHY_1M,
//     .max_skip = 0,
//     .secondary_phy = ESP_BLE_GAP_PHY_1M,
//     .sid = 2,
//     .scan_req_notif = false,
//     .own_addr_type = BLE_ADDR_TYPE_RANDOM,
//     .tx_power = EXT_ADV_TX_PWR_NO_PREFERENCE,
// };

// esp_ble_gap_ext_adv_params_t ext_adv_params_coded = {
//     .type = ESP_BLE_GAP_SET_EXT_ADV_PROP_SCANNABLE,
//     .interval_min = 0x50,
//     .interval_max = 0x50,
//     .channel_map = ADV_CHNL_ALL,
//     .filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
//     .primary_phy = ESP_BLE_GAP_PHY_1M,
//     .max_skip = 0,
//     .secondary_phy = ESP_BLE_GAP_PHY_CODED,
//     .sid = 3,
//     .scan_req_notif = false,
//     .own_addr_type = BLE_ADDR_TYPE_RANDOM,
//     .tx_power = EXT_ADV_TX_PWR_NO_PREFERENCE,
// };

// // static esp_ble_adv_params_t legacy_adv_params = {
// //     .adv_int_min        = 0x20,
// //     .adv_int_max        = 0x40,
// //     .adv_type           = ADV_TYPE_NONCONN_IND, //不可连接广播
// //     .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
// //     //.peer_addr            =
// //     //.peer_addr_type       =
// //     .channel_map        = ADV_CHNL_ALL,
// //     .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
// // };


// static uint8_t raw_adv_data_1m[] = {
//         0x02, 0x01, 0x06,
//         0x02, 0x0a, 0xeb,
//         0x11, 0x09, 'E', 'S', 'P', '_', 'M', 'U', 'L', 'T', 'I', '_', 'A',
//         'D', 'V', '_', '1', 'M'
// };

// static uint8_t raw_scan_rsp_data_2m[] = {
//         0x02, 0x01, 0x06,
//         0x02, 0x0a, 0xeb,
//         0x11, 0x09, 'E', 'S', 'P', '_', 'M', 'U', 'L', 'T', 'I', '_', 'A',
//         'D', 'V', '_', '2', 'M'
// };

// static uint8_t legacy_adv_data[] = {
//         0x02, 0x01, 0x06,
//         0x02, 0x0a, 0xeb,
//         0x14, 0x09, 'E', 'S', 'P', '_', 'M', 'U', 'L', 'T', 'I', '_', 'A',
//         'D', 'V', '_', 'C', 'O', 'D', 'E', 'D'
// };

// static uint8_t legacy_scan_rsp_data[] = {
//         0x02, 0x01, 0x06,
//         0x02, 0x0a, 0xeb,
//         0x15, 0x09, 'E', 'S', 'P', '_', 'M', 'U', 'L', 'T', 'I', '_', 'A',
//         'D', 'V', '_', 'L', 'E', 'G', 'A', 'C', 'Y'
// };

// static uint8_t raw_scan_rsp_data_coded[] = {
//         0x02, 0x01, 0x06,
//         0x02, 0x0a, 0xeb,
//         0x14, 0x09, 'E', 'S', 'P', '_', 'M', 'U', 'L', 'T', 'I', '_', 'A',
//         'D', 'V', '_', 'C', 'O', 'D', 'E', 'D'
// };

// static esp_ble_gap_ext_adv_t ext_adv[4] = {
//     // instance, duration, period
//     [0] = {0, 0, 0},
//     [1] = {1, 0, 0},
//     [2] = {2, 0, 0},
//     [3] = {3, 0, 0},
// };

// static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
// {
//     switch (event) {
//     case ESP_GAP_BLE_EXT_ADV_SET_RAND_ADDR_COMPLETE_EVT:
//         xSemaphoreGive(set_sem);
//         ESP_LOGI(LOG_TAG, "ESP_GAP_BLE_EXT_ADV_SET_RAND_ADDR_COMPLETE_EVT, status %d", param->ext_adv_set_rand_addr.status);
//         break;
//     case ESP_GAP_BLE_EXT_ADV_SET_PARAMS_COMPLETE_EVT:
//         xSemaphoreGive(set_sem);
//         ESP_LOGI(LOG_TAG, "ESP_GAP_BLE_EXT_ADV_SET_PARAMS_COMPLETE_EVT, status %d", param->ext_adv_set_params.status);
//         break;
//     case ESP_GAP_BLE_EXT_ADV_DATA_SET_COMPLETE_EVT:
//         xSemaphoreGive(set_sem);
//         ESP_LOGI(LOG_TAG, "ESP_GAP_BLE_EXT_ADV_DATA_SET_COMPLETE_EVT, status %d", param->ext_adv_data_set.status);
//         break;
//     case ESP_GAP_BLE_EXT_SCAN_RSP_DATA_SET_COMPLETE_EVT:
//         xSemaphoreGive(set_sem);
//         ESP_LOGI(LOG_TAG, "ESP_GAP_BLE_EXT_SCAN_RSP_DATA_SET_COMPLETE_EVT, status %d", param->scan_rsp_set.status);
//         break;
//     case ESP_GAP_BLE_EXT_ADV_START_COMPLETE_EVT:
//         xSemaphoreGive(set_sem);
//         ESP_LOGI(LOG_TAG, "ESP_GAP_BLE_EXT_ADV_START_COMPLETE_EVT, status %d", param->ext_adv_start.status);
//         break;
//     case ESP_GAP_BLE_EXT_ADV_STOP_COMPLETE_EVT:
//         xSemaphoreGive(set_sem);
//         ESP_LOGI(LOG_TAG, "ESP_GAP_BLE_EXT_ADV_STOP_COMPLETE_EVT, status %d", param->ext_adv_stop.status);
//         break;
//     default:
//         break;
//     }
// }
// static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
// {

// }

// //添加rid广播
// uint8_t service_BasicID_data[29] = {0};
// uint8_t service_Location_data[29] = {0};
// uint8_t service_Auth0_data[29] = {0};
// uint8_t service_Auth1_data[29] = {0};
// uint8_t service_SelfID_data[29] = {0};
// uint8_t service_System_data[29] = {0};
// uint8_t service_OperatorID_data[29] = {0};
// //添加rid广播参数
// static esp_ble_adv_data_t adv_BasicID_data = {
//     .appearance = 0x00,
//     .service_data_len = sizeof(service_BasicID_data),
//     .p_service_data = service_BasicID_data,
// };
// static esp_ble_adv_data_t adv_Location_data = {
//     .appearance = 0x00,
//     .service_data_len = sizeof(service_Location_data),
//     .p_service_data = service_Location_data,
// };
// static esp_ble_adv_data_t adv_Auth0_data = {
//     .appearance = 0x00,
//     .service_data_len = sizeof(service_Auth0_data),
//     .p_service_data = service_Auth0_data,
// };
// static esp_ble_adv_data_t adv_Auth1_data = {
//     .appearance = 0x00,
//     .service_data_len = sizeof(service_Auth1_data),
//     .p_service_data = service_Auth1_data,
// };
// static esp_ble_adv_data_t adv_SelfID_data = {
//     .appearance = 0x00,
//     .service_data_len = sizeof(service_SelfID_data),
//     .p_service_data = service_SelfID_data,
// };
// static esp_ble_adv_data_t adv_System_data = {
//     .appearance = 0x00,
//     .service_data_len = sizeof(service_System_data),
//     .p_service_data = service_System_data,
// };
// static esp_ble_adv_data_t adv_OperatorID_data = {
//     .appearance = 0x00,
//     .service_data_len = sizeof(service_OperatorID_data),
//     .p_service_data = service_OperatorID_data,
// };
// //定义rid信息
// static ODID_BasicID_encoded BasicID_enc;
// static ODID_BasicID_data BasicID;
// //static ODID_BasicID_data BasicID_out;

// static ODID_Location_encoded Location_enc;
// static ODID_Location_data Location;
// //static ODID_Location_data Location_out;

// static ODID_Auth_encoded Auth0_enc;
// static ODID_Auth_encoded Auth1_enc;
// static ODID_Auth_data Auth0;
// static ODID_Auth_data Auth1;
// // static ODID_Auth_data Auth0_out;
// // static ODID_Auth_data Auth1_out;

// static ODID_SelfID_encoded SelfID_enc;
// static ODID_SelfID_data SelfID;
// //static ODID_SelfID_data SelfID_out;

// static ODID_System_encoded System_enc;
// static ODID_System_data System_data;
// //static ODID_System_data System_out;

// static ODID_OperatorID_encoded OperatorID_enc;
// static ODID_OperatorID_data operatorID;
// //static ODID_OperatorID_data operatorID_out;


// static ODID_MessagePack_encoded pack_enc;
// static ODID_MessagePack_data pack;
// static ODID_UAS_Data uasData;
// #define  ble_rid_info  0
// //编码数据
// void fill_BasicID_encoded(void)
// {
//     odid_initBasicIDData(&BasicID);
//     BasicID.IDType = ODID_IDTYPE_CAA_REGISTRATION_ID;
//     BasicID.UAType = ODID_UATYPE_HELICOPTER_OR_MULTIROTOR;
//     char id[] = "12345678901234567890";
//     strncpy(BasicID.UASID, id, sizeof(BasicID.UASID));
//     encodeBasicIDMessage(&BasicID_enc, &BasicID);
//     // 清空数组，将所有元素设置为 0
//     memset(service_BasicID_data, 0, sizeof(service_BasicID_data));
//     // 固定前3个元素
//     service_BasicID_data[0] = 0xFA;
//     service_BasicID_data[1] = 0xFF;
//     service_BasicID_data[2] = 0x0D;
//     // 让 service_data[3] 自增到 0xFF 后清零
//     static uint8_t counter = 0;
//     service_BasicID_data[3] = counter++;
//     if (counter == 0xFF) {
//         counter = 0;
//     }
//     // 计算正确的复制长度
//     size_t basicID_len = sizeof(BasicID_enc);
//     memcpy(&service_BasicID_data[4], (uint8_t*) &BasicID_enc, basicID_len);
//     //
//     #if ble_rid_info
//         for (size_t i = 0; i < sizeof(service_BasicID_data); i++) {
//             printf("%02X ", service_BasicID_data[i]);
//         }
//     #endif
//     esp_ble_gap_ext_adv_stop(1,&ext_adv[2]);
//     // config adv data
//     esp_err_t ret = esp_ble_gap_config_ext_adv_data_raw(2, sizeof(service_BasicID_data), &service_BasicID_data[0]);
//     //esp_err_t ret = esp_ble_gap_config_adv_data(&adv_BasicID_data); // 更新数据
//     if (ret) {
//         printf("config adv data failed, error code = %x", ret);
//     }
//     esp_ble_gap_ext_adv_start(1,&ext_adv[2]);  // 重启广播
// }
// void fill_Location_encoded(void)
// {
//     if (xSemaphoreTake(gps_Mutex, portMAX_DELAY) == pdTRUE) {
//         // 访问共享资源
//         //printf("\n----------update_ble_gps_data-----\n");
//         odid_initLocationData(&Location);
//         Location.Status = ODID_STATUS_AIRBORNE;
//         Location.Direction = 215.7f;
//         Location.SpeedHorizontal = 5.4f;
//         Location.SpeedVertical = 5.25f;
//         Location.Latitude = 45.539309;
//         Location.Longitude = -122.966389;
//         Location.AltitudeBaro = 100;
//         Location.AltitudeGeo = 110;
//         Location.HeightType = ODID_HEIGHT_REF_OVER_GROUND;
//         Location.Height = 80;
//         Location.HorizAccuracy = createEnumHorizontalAccuracy(2.5f);
//         Location.VertAccuracy = createEnumVerticalAccuracy(0.5f);
//         Location.BaroAccuracy = createEnumVerticalAccuracy(1.5f);
//         Location.SpeedAccuracy = createEnumSpeedAccuracy(0.5f);
//         Location.TSAccuracy = createEnumTimestampAccuracy(0.2f);
//         Location.TimeStamp = 360.52f;
//         xSemaphoreGive(gps_Mutex);  // 释放互斥信号量
//     }
//     encodeLocationMessage(&Location_enc, &Location);
//     // 清空数组，将所有元素设置为 0
//     memset(service_Location_data, 0, sizeof(service_Location_data));
//     // 固定前3个元素
//     service_Location_data[0] = 0xFA;
//     service_Location_data[1] = 0xFF;
//     service_Location_data[2] = 0x0D;
//     // 让 service_data[3] 自增到 0xFF 后清零
//     static uint8_t counter = 0;
//     service_Location_data[3] = counter++;
//     if (counter == 0xFF) {
//         counter = 0;
//     }
//     // 根据 Location_enc 填充剩余部分
//     // 计算正确的复制长度
//     size_t Location_len = sizeof(Location_enc);
//     memcpy(&service_Location_data[4], &Location_enc, Location_len);
//     //
//     #if ble_rid_info
//         for (size_t i = 0; i < sizeof(service_Location_data); i++) {
//             printf("%02X ", service_Location_data[i]);
//         }
//     #endif
//     esp_ble_gap_ext_adv_stop(1,&ext_adv[2]);  // 停止当前广播
//     // config adv data
//     esp_err_t ret = esp_ble_gap_config_ext_adv_data_raw(2, sizeof(service_Location_data), &service_Location_data[0]);
//     //esp_err_t ret = esp_ble_gap_config_adv_data(&adv_Location_data);
//     if (ret) {
//         printf("config adv data failed, error code = %x", ret);
//     }
//     esp_ble_gap_ext_adv_start(1,&ext_adv[2]);  // 重启广播
// }
// void fill_AuthData0_encoded(void)
// { 
//     odid_initAuthData(&Auth0);
//     Auth0.AuthType = ODID_AUTH_UAS_ID_SIGNATURE;
//     Auth0.DataPage = 0;
//     Auth0.LastPageIndex = 1;
//     Auth0.Length = 40;
//     Auth0.Timestamp = 28000000;
//     char auth0_data[] = "12345678901234567";
//     memcpy(Auth0.AuthData, auth0_data, MINIMUM(sizeof(auth0_data), sizeof(Auth0.AuthData)));
//     encodeAuthMessage(&Auth0_enc, &Auth0);
//     // 清空数组，将所有元素设置为 0
//     memset(service_Auth0_data, 0, sizeof(service_Auth0_data));
//     // 固定前3个元素
//     service_Auth0_data[0] = 0xFA;
//     service_Auth0_data[1] = 0xFF;
//     service_Auth0_data[2] = 0x0D;
//     // 让 service_data[3] 自增到 0xFF 后清零
//     static uint8_t counter = 0;
//     service_Auth0_data[3] = counter++;
//     if (counter == 0xFF) {
//         counter = 0;
//     }
//     // 计算正确的复制长度
//     size_t Auth0_len = sizeof(Auth0_enc);
//     memcpy(&service_Auth0_data[4], (uint8_t*) &Auth0_enc, Auth0_len);
//     //
//     #if ble_rid_info
//         for (size_t i = 0; i < sizeof(service_Auth0_data); i++) {
//             printf("%02X ", service_Auth0_data[i]);
//         }
//     #endif
//     esp_ble_gap_stop_advertising();  // 停止当前广播
//     // config adv data
//     esp_err_t ret = esp_ble_gap_config_adv_data(&adv_Auth0_data);
//     if (ret) {
//         printf("config adv data failed, error code = %x", ret);
//     }
//     esp_ble_gap_start_advertising(&legacy_adv_params);  // 重启广播
// }
// void fill_AuthData1_encoded(void)
// {
//     odid_initAuthData(&Auth1);
//     Auth1.AuthType = ODID_AUTH_UAS_ID_SIGNATURE;
//     Auth1.DataPage = 1;
//     char auth1_data[] = "12345678901234567890123";
//     memcpy(Auth1.AuthData, auth1_data, MINIMUM(sizeof(auth1_data), sizeof(Auth1.AuthData)));
//     encodeAuthMessage(&Auth1_enc, &Auth1);
//     // 清空数组，将所有元素设置为 0
//     memset(service_Auth1_data, 0, sizeof(service_Auth1_data));
//     // 固定前3个元素
//     service_Auth1_data[0] = 0xFA;
//     service_Auth1_data[1] = 0xFF;
//     service_Auth1_data[2] = 0x0D;
//     // 让 service_data[3] 自增到 0xFF 后清零
//     static uint8_t counter = 0;
//     service_Auth1_data[3] = counter++;
//     if (counter == 0xFF) {
//         counter = 0;
//     }
//     // 计算正确的复制长度
//     size_t Auth1_len = sizeof(Auth1_enc);
//     memcpy(&service_Auth1_data[4], (uint8_t*) &Auth1_enc, Auth1_len);
//     //
//     #if ble_rid_info
//         for (size_t i = 0; i < sizeof(service_Auth1_data); i++) {
//             printf("%02X ", service_Auth1_data[i]);
//         }
//     #endif
//     esp_ble_gap_stop_advertising();  // 停止当前广播
//     // config adv data
//     esp_err_t ret = esp_ble_gap_config_adv_data(&adv_Auth1_data);
//     if (ret) {
//         printf("config adv data failed, error code = %x", ret);
//     }
//     esp_ble_gap_start_advertising(&legacy_adv_params);  // 重启广播
// }
// void fill_SelfID_encoded(void)
// {
//     odid_initSelfIDData(&SelfID);
//     SelfID.DescType = ODID_DESC_TYPE_TEXT;
//     char description[] = "DronesRUS: Real Estate";
//     strncpy(SelfID.Desc, description, sizeof(SelfID.Desc));
//     encodeSelfIDMessage(&SelfID_enc, &SelfID);
//     // 清空数组，将所有元素设置为 0
//     memset(service_SelfID_data, 0, sizeof(service_SelfID_data));
//     // 固定前3个元素
//     service_SelfID_data[0] = 0xFA;
//     service_SelfID_data[1] = 0xFF;
//     service_SelfID_data[2] = 0x0D;
//     // 让 service_data[3] 自增到 0xFF 后清零
//     static uint8_t counter = 0;
//     service_SelfID_data[3] = counter++;
//     if (counter == 0xFF) {
//         counter = 0;
//     }
//     // 计算正确的复制长度
//     size_t SelfID_len = sizeof(SelfID_enc);
//     memcpy(&service_SelfID_data[4], (uint8_t*) &SelfID_enc, SelfID_len);
//     //
//     #if ble_rid_info
//         for (size_t i = 0; i < sizeof(service_SelfID_data); i++) {
//             printf("%02X ", service_SelfID_data[i]);
//         }
//     #endif
//     esp_ble_gap_stop_advertising();  // 停止当前广播
//     // config adv data
//     esp_err_t ret = esp_ble_gap_config_adv_data(&adv_SelfID_data);
//     if (ret) {
//         printf("config adv data failed, error code = %x", ret);
//     }
//     esp_ble_gap_start_advertising(&legacy_adv_params);  // 重启广播
// }
// void fill_System_encoded(void)
// {
//     odid_initSystemData(&System_data);
//     System_data.OperatorLocationType = ODID_OPERATOR_LOCATION_TYPE_TAKEOFF;
//     System_data.ClassificationType = ODID_CLASSIFICATION_TYPE_EU;
//     System_data.OperatorLatitude = Location.Latitude + 0.00001;
//     System_data.OperatorLongitude = Location.Longitude + 0.00001;
//     System_data.AreaCount = 35;
//     System_data.AreaRadius = 75;
//     System_data.AreaCeiling = 176.9f;
//     System_data.AreaFloor = 41.7f;
//     System_data.CategoryEU = ODID_CATEGORY_EU_SPECIFIC;
//     System_data.ClassEU = ODID_CLASS_EU_CLASS_3;
//     System_data.OperatorAltitudeGeo = 20.5f;
//     System_data.Timestamp = 28000000;
//     encodeSystemMessage(&System_enc, &System_data);
//     // 清空数组，将所有元素设置为 0
//     memset(service_System_data, 0, sizeof(service_System_data));
//     // 固定前3个元素
//     service_System_data[0] = 0xFA;
//     service_System_data[1] = 0xFF;
//     service_System_data[2] = 0x0D;
//     // 让 service_data[3] 自增到 0xFF 后清零
//     static uint8_t counter = 0;
//     service_System_data[3] = counter++;
//     if (counter == 0xFF) {
//         counter = 0;
//     }
//     // 计算正确的复制长度
//     size_t System_len = sizeof(System_enc);
//     memcpy(&service_System_data[4], (uint8_t*) &System_enc, System_len);
//     //
//     #if ble_rid_info
//         for (size_t i = 0; i < sizeof(service_System_data); i++) {
//             printf("%02X ", service_System_data[i]);
//         }
//     #endif
//     esp_ble_gap_stop_advertising();  // 停止当前广播
//     // config adv data
//     esp_err_t ret = esp_ble_gap_config_adv_data(&adv_System_data);
//     if (ret) {
//         printf("config adv data failed, error code = %x", ret);
//     }
//     esp_ble_gap_start_advertising(&legacy_adv_params);  // 重启广播
// }
// void fill_OperatorID_encoded(void)
// {
//     odid_initOperatorIDData(&operatorID);
//     operatorID.OperatorIdType = ODID_OPERATOR_ID;
//     char operatorId[] = "98765432100123456789";
//     strncpy(operatorID.OperatorId, operatorId, sizeof(operatorID.OperatorId));
//     encodeOperatorIDMessage(&OperatorID_enc, &operatorID);
//     // 清空数组，将所有元素设置为 0
//     memset(service_OperatorID_data, 0, sizeof(service_OperatorID_data));
//     // 固定前3个元素
//     service_OperatorID_data[0] = 0xFA;
//     service_OperatorID_data[1] = 0xFF;
//     service_OperatorID_data[2] = 0x0D;
//     // 让 service_data[3] 自增到 0xFF 后清零
//     static uint8_t counter = 0;
//     service_OperatorID_data[3] = counter++;
//     if (counter == 0xFF) {
//         counter = 0;
//     }
//     // 计算正确的复制长度
//     size_t OperatorID_len = sizeof(OperatorID_enc);
//     memcpy(&service_OperatorID_data[4], (uint8_t*) &OperatorID_enc, OperatorID_len);
//     //
//     #if ble_rid_info
//         for (size_t i = 0; i < sizeof(service_OperatorID_data); i++) {
//             printf("%02X ", service_OperatorID_data[i]);
//         }
//     #endif
//     esp_ble_gap_stop_advertising();  // 停止当前广播
//     // config adv data
//     esp_err_t ret = esp_ble_gap_config_adv_data(&adv_OperatorID_data);
//     if (ret) {
//         printf("config adv data failed, error code = %x", ret);
//     }
//     esp_ble_gap_start_advertising(&legacy_adv_params);  // 重启广播

// }

// uint8_t ble_enable;
// /* 在app_main函数前添加任务实现 */
// static void ble_adv_task(void *pvParameters)
// {
//     static int count = 0;
//     while (1) {
//         if(ble_enable)
//         {   
//             // count++;
//             // if(count == 1)
//             // {
//             //     fill_BasicID_encoded();
//             // }      
//             // else if(count == 2)
//             // {
//             //     fill_Location_encoded();
//             //     count = 0;
//             // }
//             // else if(count == 3)
//             // {
//             //     fill_AuthData0_encoded();
//             // }
//             // else if(count == 4)
//             // {
//             //     fill_AuthData1_encoded();
//             // }  
//             // else if(count == 5)
//             // {
//             //     fill_SelfID_encoded();
//             // }  
//             // else if(count == 6)
//             // {
//             //     fill_System_encoded();
//             // } 
//             // else if(count == 7)
//             // {
//             //     fill_OperatorID_encoded();
//             //     count = 0;
//             // }     
//         }
//         vTaskDelay(pdMS_TO_TICKS(200));  // ms
//     }
// }

// //初始化ble
// int ble_init(void)
// {
//     esp_err_t ret;

//     ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

//     esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
//     ret = esp_bt_controller_init(&bt_cfg);
//     if (ret) {
//         ESP_LOGE(LOG_TAG, "%s initialize controller failed: %s", __func__, esp_err_to_name(ret));
//         return -1;
//     }

//     ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
//     if (ret) {
//         ESP_LOGE(LOG_TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
//         return -1;
//     }

//     ret = esp_bluedroid_init();
//     if (ret) {
//         ESP_LOGE(LOG_TAG, "%s init bluetooth failed: %s", __func__, esp_err_to_name(ret));
//         return -1;
//     }
//     ret = esp_bluedroid_enable();
//     if (ret) {
//         ESP_LOGE(LOG_TAG, "%s enable bluetooth failed: %s", __func__, esp_err_to_name(ret));
//         return -1;
//     }
//     ret = esp_ble_gap_register_callback(gap_event_handler);
//     if (ret){
//         ESP_LOGE(LOG_TAG, "gap register error, error code = %x", ret);
//         return -1;
//     }
//     // 注册 GATT 回调函数，处理所有的 GATT 事件
//     ret = esp_ble_gatts_register_callback(gatts_event_handler);
//     if (ret){
//         ESP_LOGE(LOG_TAG, "gatts register error, error code = %x", ret);
//         return -1;
//     }
   
//     esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
//     if (local_mtu_ret){
//         ESP_LOGE(LOG_TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
//         return -1;
//     }
//     vTaskDelay(200 / portTICK_PERIOD_MS);

//     set_sem = xSemaphoreCreateBinary();
//     // 1M phy extend adv, Connectable advertising
//     FUNC_SEND_WAIT_SEM(esp_ble_gap_ext_adv_set_params(0, &ext_adv_params_1M), set_sem);
//     FUNC_SEND_WAIT_SEM(esp_ble_gap_ext_adv_set_rand_addr(0, addr_1m), set_sem);
//     FUNC_SEND_WAIT_SEM(esp_ble_gap_config_ext_adv_data_raw(0, sizeof(raw_adv_data_1m), &raw_adv_data_1m[0]), set_sem);

//     // 2M phy extend adv, Scannable advertising
//     FUNC_SEND_WAIT_SEM(esp_ble_gap_ext_adv_set_params(1, &ext_adv_params_2M), set_sem);
//     FUNC_SEND_WAIT_SEM(esp_ble_gap_ext_adv_set_rand_addr(1, addr_2m), set_sem);
//     FUNC_SEND_WAIT_SEM(esp_ble_gap_config_ext_scan_rsp_data_raw(1, sizeof(raw_scan_rsp_data_2m), raw_scan_rsp_data_2m), set_sem);

//     // 1M phy legacy adv, ADV_IND
//     FUNC_SEND_WAIT_SEM(esp_ble_gap_ext_adv_set_params(2, &legacy_adv_params), set_sem);
//     FUNC_SEND_WAIT_SEM(esp_ble_gap_ext_adv_set_rand_addr(2, addr_legacy), set_sem);
//     FUNC_SEND_WAIT_SEM(esp_ble_gap_config_ext_adv_data_raw(2, sizeof(legacy_adv_data), &legacy_adv_data[0]), set_sem);
//     FUNC_SEND_WAIT_SEM(esp_ble_gap_config_ext_scan_rsp_data_raw(2, sizeof(legacy_scan_rsp_data), &legacy_scan_rsp_data[0]), set_sem);

//     // coded phy extend adv, Scannable advertising
//     FUNC_SEND_WAIT_SEM(esp_ble_gap_ext_adv_set_params(3, &ext_adv_params_coded), set_sem);
//     FUNC_SEND_WAIT_SEM(esp_ble_gap_ext_adv_set_rand_addr(3, addr_coded), set_sem);
//     FUNC_SEND_WAIT_SEM(esp_ble_gap_config_ext_scan_rsp_data_raw(3, sizeof(raw_scan_rsp_data_coded), &raw_scan_rsp_data_coded[0]), set_sem);

//     // start all adv
//     FUNC_SEND_WAIT_SEM(esp_ble_gap_ext_adv_start(4, &ext_adv[0]), set_sem);

//     ble_enable = 1;
//     BaseType_t xReturn;
//     xReturn = xTaskCreatePinnedToCore(ble_adv_task,"ble_adv_task",8192,NULL,14,NULL, tskNO_AFFINITY);
//     if(xReturn != pdPASS) 
//     {
//         printf("xTaskCreatePinnedToCore ble_adv_task error!\r\n");
//     }
//     return 1;
// }






#include "ble_peripheral.h"

#define LOG_TAG "BLE_ADV"

#define FUNC_SEND_WAIT_SEM(func, sem) do {\
        esp_err_t __err_rc = (func);\
        if (__err_rc != ESP_OK) { \
            ESP_LOGE(LOG_TAG, "%s, message send fail, error = %d", __func__, __err_rc); \
        } \
        xSemaphoreTake(sem, portMAX_DELAY); \
} while(0);

static SemaphoreHandle_t set_sem = NULL;

uint8_t addr_1m[6] = {0xc0, 0xde, 0x52, 0x00, 0x00, 0x01};
uint8_t addr_2m[6] = {0xc0, 0xde, 0x52, 0x00, 0x00, 0x02};
uint8_t addr_legacy[6] = {0xc0, 0xde, 0x52, 0x00, 0x00, 0x03};
uint8_t addr_coded[6] = {0xc0, 0xde, 0x52, 0x00, 0x00, 0x04};

esp_ble_gap_ext_adv_params_t ext_adv_params_1M = {
    .type = ESP_BLE_GAP_SET_EXT_ADV_PROP_CONNECTABLE,
    .interval_min = 0x30,
    .interval_max = 0x30,
    .channel_map = ADV_CHNL_ALL,
    .filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
    .primary_phy = ESP_BLE_GAP_PHY_1M,
    .max_skip = 0,
    .secondary_phy = ESP_BLE_GAP_PHY_1M,
    .sid = 0,
    .scan_req_notif = false,
    .own_addr_type = BLE_ADDR_TYPE_RANDOM,
    .tx_power = EXT_ADV_TX_PWR_NO_PREFERENCE,
};

esp_ble_gap_ext_adv_params_t ext_adv_params_2M = {
    .type = ESP_BLE_GAP_SET_EXT_ADV_PROP_SCANNABLE,
    .interval_min = 0x40,
    .interval_max = 0x40,
    .channel_map = ADV_CHNL_ALL,
    .filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
    .primary_phy = ESP_BLE_GAP_PHY_1M,
    .max_skip = 0,
    .secondary_phy = ESP_BLE_GAP_PHY_2M,
    .sid = 1,
    .scan_req_notif = false,
    .own_addr_type = BLE_ADDR_TYPE_RANDOM,
    .tx_power = EXT_ADV_TX_PWR_NO_PREFERENCE,
};
//传统ble蓝牙4.0
esp_ble_gap_ext_adv_params_t legacy_adv_params = {
    .type = ESP_BLE_GAP_SET_EXT_ADV_PROP_LEGACY_IND,
    .interval_min = 0x45,
    .interval_max = 0x45,
    .channel_map = ADV_CHNL_ALL,
    .filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
    .primary_phy = ESP_BLE_GAP_PHY_1M,
    .max_skip = 0,
    .secondary_phy = ESP_BLE_GAP_PHY_CODED,
    .sid = 2,
    .scan_req_notif = false,
    .own_addr_type = BLE_ADDR_TYPE_RANDOM,
    .tx_power = EXT_ADV_TX_PWR_NO_PREFERENCE,
};
//蓝牙ble拓展包5.0
esp_ble_gap_ext_adv_params_t ext_adv_params_coded = {
    .type = ESP_BLE_GAP_SET_EXT_ADV_PROP_CONNECTABLE,
    .interval_min = 0x50,
    .interval_max = 0x50,
    .channel_map = ADV_CHNL_ALL,
    .filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
    .primary_phy = ESP_BLE_GAP_PHY_1M,
    .max_skip = 0,
    .secondary_phy = ESP_BLE_GAP_PHY_1M,
    .sid = 3,
    .scan_req_notif = false,
    .own_addr_type = BLE_ADDR_TYPE_RANDOM,
    .tx_power = EXT_ADV_TX_PWR_NO_PREFERENCE,
};

static uint8_t raw_adv_data_1m[] = {
        0x02, 0x01, 0x06,
        0x02, 0x0a, 0xeb,
        0x11, 0x09, 'E', 'S', 'P', '_', 'M', 'U', 'L', 'T', 'I', '_', 'A',
        'D', 'V', '_', '1', 'M'
};

static uint8_t raw_scan_rsp_data_2m[] = {
        0x02, 0x01, 0x06,// Flags
        0x02, 0x0a, 0xeb, // Tx Power
        0x11, 0x09, 'E', 'S', 'P', '_', 'M', 'U', 'L', 'T', 'I', '_', 'A',
        'D', 'V', '_', '2', 'M'
};

static uint8_t legacy_adv_data[] = {
    0x02, 0x01, 0x06,                     // Flags
    //0x02, 0x0a, 0xeb,                     // Tx Power
    //0x05, 0x16, 0xFA, 0xFF, 0x34, 0x12,   // 修正后的服务数据（长度=5）
    //0x0B, 0x0E, 'E','S','P','_','M','U','L','T','I','_','L','E','G','A','C','Y', //蓝牙名字0XB 长度0x0E
};

static uint8_t legacy_scan_rsp_data[] = {
    0x02, 0x01, 0x06,                     // Flags
    //0x02, 0x0a, 0xeb,                     // Tx Power
    //0x05, 0x16, 0xFA, 0xFF, 0x34, 0x12,   // 修正后的服务数据（长度=5）
    //0x0B, 0x09, 'E','S','P','_','M','U','L','T','I','_','L','E','G','A','C','Y',
};

static uint8_t raw_scan_rsp_data_coded[] = {
        //0x02, 0x01, 0x06,   // Flags
        //0x02, 0x0a, 0xeb,   // Tx Power
        0x05, 0x16, 0xFA, 0xFF, 0x34, 0x12,   // 修正后的服务数据（长度=5）
        //0x14, 0x09, 'E', 'S', 'P', '_', 'M', 'U', 'L', 'T', 'I', '_', 'A','D', 'V', '_', 'C', 'O', 'D', 'E', 'D'//local name
};

static esp_ble_gap_ext_adv_t ext_adv[4] = {
    // instance, duration, period
    [0] = {0, 0, 0},
    [1] = {1, 0, 0},
    [2] = {2, 0, 0},
    [3] = {3, 0, 0},
};

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_GAP_BLE_EXT_ADV_SET_RAND_ADDR_COMPLETE_EVT:
        xSemaphoreGive(set_sem);
        ESP_LOGI(LOG_TAG, "ESP_GAP_BLE_EXT_ADV_SET_RAND_ADDR_COMPLETE_EVT, status %d", param->ext_adv_set_rand_addr.status);
        break;
    case ESP_GAP_BLE_EXT_ADV_SET_PARAMS_COMPLETE_EVT:
        xSemaphoreGive(set_sem);
        ESP_LOGI(LOG_TAG, "ESP_GAP_BLE_EXT_ADV_SET_PARAMS_COMPLETE_EVT, status %d", param->ext_adv_set_params.status);
        break;
    case ESP_GAP_BLE_EXT_ADV_DATA_SET_COMPLETE_EVT:
        xSemaphoreGive(set_sem);
        ESP_LOGI(LOG_TAG, "ESP_GAP_BLE_EXT_ADV_DATA_SET_COMPLETE_EVT, status %d", param->ext_adv_data_set.status);
        break;
    case ESP_GAP_BLE_EXT_SCAN_RSP_DATA_SET_COMPLETE_EVT:
        xSemaphoreGive(set_sem);
        ESP_LOGI(LOG_TAG, "ESP_GAP_BLE_EXT_SCAN_RSP_DATA_SET_COMPLETE_EVT, status %d", param->scan_rsp_set.status);
        break;
    case ESP_GAP_BLE_EXT_ADV_START_COMPLETE_EVT:
        xSemaphoreGive(set_sem);
        ESP_LOGI(LOG_TAG, "ESP_GAP_BLE_EXT_ADV_START_COMPLETE_EVT, status %d", param->ext_adv_start.status);
        break;
    case ESP_GAP_BLE_EXT_ADV_STOP_COMPLETE_EVT:
        xSemaphoreGive(set_sem);
        ESP_LOGI(LOG_TAG, "ESP_GAP_BLE_EXT_ADV_STOP_COMPLETE_EVT, status %d", param->ext_adv_stop.status);
        break;
    default:
        break;
    }
}

static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{

}

//定义rid信息
static ODID_BasicID_encoded BasicID_enc;
static ODID_BasicID_data BasicID;

static ODID_Location_encoded Location_enc;
static ODID_Location_data Location;

static ODID_Auth_encoded Auth0_enc;
static ODID_Auth_encoded Auth1_enc;
static ODID_Auth_data Auth0;
static ODID_Auth_data Auth1;

static ODID_SelfID_encoded SelfID_enc;
static ODID_SelfID_data SelfID;

static ODID_System_encoded System_enc;
static ODID_System_data System_data;

static ODID_OperatorID_encoded OperatorID_enc;
static ODID_OperatorID_data operatorID;

static ODID_MessagePack_encoded pack_enc;
static ODID_MessagePack_data pack;
static ODID_UAS_Data uasData;

//添加rid传统ble广播
static uint8_t service_BasicID_data[31] = {0};
static uint8_t service_Location_data[31] = {0};
static uint8_t service_Auth0_data[31] = {0};
static uint8_t service_Auth1_data[31] = {0};
static uint8_t service_SelfID_data[31] = {0};
static uint8_t service_System_data[31] = {0};
static uint8_t service_OperatorID_data[31] = {0};
static uint8_t service_REMOTEID_data[184] = {0};
#define  ble_rid_info  0
//编码数据
static void fill_BasicID_encoded(void)
{
    odid_initBasicIDData(&BasicID);
    BasicID.IDType = ODID_IDTYPE_CAA_REGISTRATION_ID;
    BasicID.UAType = ODID_UATYPE_HELICOPTER_OR_MULTIROTOR;
    char id[] = "12345678901234567890";
    strncpy(BasicID.UASID, id, sizeof(BasicID.UASID));
    encodeBasicIDMessage(&BasicID_enc, &BasicID);
    // 清空数组，将所有元素设置为 0
    memset(service_BasicID_data, 0, sizeof(service_BasicID_data));
    // 固定前5个元素
    service_BasicID_data[0] = 0x1E;
    service_BasicID_data[1] = 0x16;
    service_BasicID_data[2] = 0xFA;
    service_BasicID_data[3] = 0xFF;
    service_BasicID_data[4] = 0x0D;
    // 让 service_data[3] 自增到 0xFF 后清零
    static uint8_t counter = 0;
    service_BasicID_data[5] = counter++;
    if (counter == 0xFF) {
        counter = 0;
    }
    // 计算正确的复制长度
    size_t basicID_len = sizeof(BasicID_enc);
    memcpy(&service_BasicID_data[6], (uint8_t*) &BasicID_enc, basicID_len);
    //
    #if ble_rid_info
        for (size_t i = 0; i < sizeof(service_BasicID_data); i++) {
            printf("%02X ", service_BasicID_data[i]);
        }
    #endif
    esp_err_t ret;
    ret = esp_ble_gap_ext_adv_stop(1,&ext_adv[2]);  // 停止当前广播
    if (ret) {
        printf("stop adv data failed, error code = %x", ret);
    }
    // config adv data
    ret = esp_ble_gap_config_ext_adv_data_raw(2, sizeof(service_BasicID_data), &service_BasicID_data[0]);
    if (ret) {
        printf("config adv data failed, error code = %x", ret);
    }
    ret = esp_ble_gap_ext_adv_start(1,&ext_adv[2]);  // 重启广播
    if (ret) {
        printf("start adv data failed, error code = %x", ret);
    }
}
static void fill_Location_encoded(void)
{
    if (xSemaphoreTake(gps_Mutex, portMAX_DELAY) == pdTRUE) {
        // 访问共享资源
        //printf("\n----------update_ble_gps_data-----\n");
        odid_initLocationData(&Location);
        Location.Status = ODID_STATUS_AIRBORNE;
        Location.Direction = 215.7f;
        Location.SpeedHorizontal = 5.4f;
        Location.SpeedVertical = 5.25f;
        Location.Latitude = 45.539309;
        Location.Longitude = -122.966389;
        Location.AltitudeBaro = 100;
        Location.AltitudeGeo = 110;
        Location.HeightType = ODID_HEIGHT_REF_OVER_GROUND;
        Location.Height = 80;
        Location.HorizAccuracy = createEnumHorizontalAccuracy(2.5f);
        Location.VertAccuracy = createEnumVerticalAccuracy(0.5f);
        Location.BaroAccuracy = createEnumVerticalAccuracy(1.5f);
        Location.SpeedAccuracy = createEnumSpeedAccuracy(0.5f);
        Location.TSAccuracy = createEnumTimestampAccuracy(0.2f);
        Location.TimeStamp = 360.52f;
        xSemaphoreGive(gps_Mutex);  // 释放互斥信号量
    }
    encodeLocationMessage(&Location_enc, &Location);
    // 清空数组，将所有元素设置为 0
    memset(service_Location_data, 0, sizeof(service_Location_data));
    // 固定前5个元素
    service_Location_data[0] = 0x1E;
    service_Location_data[1] = 0x16;
    service_Location_data[2] = 0xFA;
    service_Location_data[3] = 0xFF;
    service_Location_data[4] = 0x0D;
    // 让 service_data[3] 自增到 0xFF 后清零
    static uint8_t counter = 0;
    service_Location_data[5] = counter++;
    if (counter == 0xFF) {
        counter = 0;
    }
    // 根据 Location_enc 填充剩余部分
    // 计算正确的复制长度
    size_t Location_len = sizeof(Location_enc);
    memcpy(&service_Location_data[6], &Location_enc, Location_len);
    //
    #if ble_rid_info
        for (size_t i = 0; i < sizeof(service_Location_data); i++) {
            printf("%02X ", service_Location_data[i]);
        }
    #endif
    esp_err_t ret;
    ret = esp_ble_gap_ext_adv_stop(1,&ext_adv[2]);  // 停止当前广播
    if (ret) {
        printf("stop adv data failed, error code = %x", ret);
    }
    // config adv data
    ret = esp_ble_gap_config_ext_adv_data_raw(2, sizeof(service_Location_data), &service_Location_data[0]);
    if (ret) {
        printf("config adv data failed, error code = %x", ret);
    }
    ret = esp_ble_gap_ext_adv_start(1,&ext_adv[2]);  // 重启广播
    if (ret) {
        printf("start adv data failed, error code = %x", ret);
    }
}
static void fill_AuthData0_encoded(void)
{ 
    odid_initAuthData(&Auth0);
    Auth0.AuthType = ODID_AUTH_UAS_ID_SIGNATURE;
    Auth0.DataPage = 0;
    Auth0.LastPageIndex = 1;
    Auth0.Length = 40;
    Auth0.Timestamp = 28000000;
    char auth0_data[] = "12345678901234567";
    memcpy(Auth0.AuthData, auth0_data, MINIMUM(sizeof(auth0_data), sizeof(Auth0.AuthData)));
    encodeAuthMessage(&Auth0_enc, &Auth0);
    // 清空数组，将所有元素设置为 0
    memset(service_Auth0_data, 0, sizeof(service_Auth0_data));
    // 固定前5个元素
    service_Auth0_data[0] = 0x1E;
    service_Auth0_data[1] = 0x16;
    service_Auth0_data[2] = 0xFA;
    service_Auth0_data[3] = 0xFF;
    service_Auth0_data[4] = 0x0D;
    // 让 service_data[5] 自增到 0xFF 后清零
    static uint8_t counter = 0;
    service_Auth0_data[5] = counter++;
    if (counter == 0xFF) {
        counter = 0;
    }
    // 计算正确的复制长度
    size_t Auth0_len = sizeof(Auth0_enc);
    memcpy(&service_Auth0_data[6], (uint8_t*) &Auth0_enc, Auth0_len);
    //
    #if ble_rid_info
        for (size_t i = 0; i < sizeof(service_Auth0_data); i++) {
            printf("%02X ", service_Auth0_data[i]);
        }
    #endif
    esp_err_t ret;
    ret = esp_ble_gap_ext_adv_stop(1,&ext_adv[2]);  // 停止当前广播
    if (ret) {
        printf("stop adv data failed, error code = %x", ret);
    }
    // config adv data
    ret = esp_ble_gap_config_ext_adv_data_raw(2, sizeof(service_Auth0_data), &service_Auth0_data[0]);
    if (ret) {
        printf("config adv data failed, error code = %x", ret);
    }
    ret = esp_ble_gap_ext_adv_start(1,&ext_adv[2]);  // 重启广播
    if (ret) {
        printf("start adv data failed, error code = %x", ret);
    }
}
static void fill_AuthData1_encoded(void)
{
    odid_initAuthData(&Auth1);
    Auth1.AuthType = ODID_AUTH_UAS_ID_SIGNATURE;
    Auth1.DataPage = 1;
    char auth1_data[] = "12345678901234567890123";
    memcpy(Auth1.AuthData, auth1_data, MINIMUM(sizeof(auth1_data), sizeof(Auth1.AuthData)));
    encodeAuthMessage(&Auth1_enc, &Auth1);
    // 清空数组，将所有元素设置为 0
    memset(service_Auth1_data, 0, sizeof(service_Auth1_data));
    // 固定前5个元素
    service_Auth1_data[0] = 0x1E;
    service_Auth1_data[1] = 0x16;
    service_Auth1_data[2] = 0xFA;
    service_Auth1_data[3] = 0xFF;
    service_Auth1_data[4] = 0x0D;
    // 让 service_data[3] 自增到 0xFF 后清零
    static uint8_t counter = 0;
    service_Auth1_data[5] = counter++;
    if (counter == 0xFF) {
        counter = 0;
    }
    // 计算正确的复制长度
    size_t Auth1_len = sizeof(Auth1_enc);
    memcpy(&service_Auth1_data[6], (uint8_t*) &Auth1_enc, Auth1_len);
    //
    #if ble_rid_info
        for (size_t i = 0; i < sizeof(service_Auth1_data); i++) {
            printf("%02X ", service_Auth1_data[i]);
        }
    #endif
    esp_err_t ret;
    ret = esp_ble_gap_ext_adv_stop(1,&ext_adv[2]);  // 停止当前广播
    if (ret) {
        printf("stop adv data failed, error code = %x", ret);
    }
    // config adv data
    ret = esp_ble_gap_config_ext_adv_data_raw(2, sizeof(service_Auth1_data), &service_Auth1_data[0]);
    if (ret) {
        printf("config adv data failed, error code = %x", ret);
    }
    ret = esp_ble_gap_ext_adv_start(1,&ext_adv[2]);  // 重启广播
    if (ret) {
        printf("start adv data failed, error code = %x", ret);
    }
}
static void fill_SelfID_encoded(void)
{
    odid_initSelfIDData(&SelfID);
    SelfID.DescType = ODID_DESC_TYPE_TEXT;
    char description[] = "DronesRUS: Real Estate";
    strncpy(SelfID.Desc, description, sizeof(SelfID.Desc));
    encodeSelfIDMessage(&SelfID_enc, &SelfID);
    // 清空数组，将所有元素设置为 0
    memset(service_SelfID_data, 0, sizeof(service_SelfID_data));
    // 固定前5个元素
    service_SelfID_data[0] = 0x1E;
    service_SelfID_data[1] = 0x16;
    service_SelfID_data[2] = 0xFA;
    service_SelfID_data[3] = 0xFF;
    service_SelfID_data[4] = 0x0D;
    // 让 service_data[3] 自增到 0xFF 后清零
    static uint8_t counter = 0;
    service_SelfID_data[5] = counter++;
    if (counter == 0xFF) {
        counter = 0;
    }
    // 计算正确的复制长度
    size_t SelfID_len = sizeof(SelfID_enc);
    memcpy(&service_SelfID_data[6], (uint8_t*) &SelfID_enc, SelfID_len);
    //
    #if ble_rid_info
        for (size_t i = 0; i < sizeof(service_SelfID_data); i++) {
            printf("%02X ", service_SelfID_data[i]);
        }
    #endif
    esp_err_t ret;
    ret = esp_ble_gap_ext_adv_stop(1,&ext_adv[2]);  // 停止当前广播
    if (ret) {
        printf("stop adv data failed, error code = %x", ret);
    }
    // config adv data
    ret = esp_ble_gap_config_ext_adv_data_raw(2, sizeof(service_SelfID_data), &service_SelfID_data[0]);
    if (ret) {
        printf("config adv data failed, error code = %x", ret);
    }
    ret = esp_ble_gap_ext_adv_start(1,&ext_adv[2]);  // 重启广播
    if (ret) {
        printf("start adv data failed, error code = %x", ret);
    }
}
static void fill_System_encoded(void)
{
    odid_initSystemData(&System_data);
    System_data.OperatorLocationType = ODID_OPERATOR_LOCATION_TYPE_TAKEOFF;
    System_data.ClassificationType = ODID_CLASSIFICATION_TYPE_EU;
    System_data.OperatorLatitude = Location.Latitude + 0.00001;
    System_data.OperatorLongitude = Location.Longitude + 0.00001;
    System_data.AreaCount = 35;
    System_data.AreaRadius = 75;
    System_data.AreaCeiling = 176.9f;
    System_data.AreaFloor = 41.7f;
    System_data.CategoryEU = ODID_CATEGORY_EU_SPECIFIC;
    System_data.ClassEU = ODID_CLASS_EU_CLASS_3;
    System_data.OperatorAltitudeGeo = 20.5f;
    System_data.Timestamp = 28000000;
    encodeSystemMessage(&System_enc, &System_data);
    // 清空数组，将所有元素设置为 0
    memset(service_System_data, 0, sizeof(service_System_data));
    // 固定前5个元素
    service_System_data[0] = 0x1E;
    service_System_data[1] = 0x16;   
    service_System_data[2] = 0xFA;
    service_System_data[3] = 0xFF;
    service_System_data[4] = 0x0D;
    // 让 service_data[3] 自增到 0xFF 后清零
    static uint8_t counter = 0;
    service_System_data[5] = counter++;
    if (counter == 0xFF) {
        counter = 0;
    }
    // 计算正确的复制长度
    size_t System_len = sizeof(System_enc);
    memcpy(&service_System_data[6], (uint8_t*) &System_enc, System_len);
    //
    #if ble_rid_info
        for (size_t i = 0; i < sizeof(service_System_data); i++) {
            printf("%02X ", service_System_data[i]);
        }
    #endif
    esp_err_t ret;
    ret = esp_ble_gap_ext_adv_stop(1,&ext_adv[2]);  // 停止当前广播
    if (ret) {
        printf("stop adv data failed, error code = %x", ret);
    }
    // config adv data
    ret = esp_ble_gap_config_ext_adv_data_raw(2, sizeof(service_System_data), &service_System_data[0]);
    if (ret) {
        printf("config adv data failed, error code = %x", ret);
    }
    ret = esp_ble_gap_ext_adv_start(1,&ext_adv[2]);  // 重启广播
    if (ret) {
        printf("start adv data failed, error code = %x", ret);
    }
}
static void fill_OperatorID_encoded(void)
{
    odid_initOperatorIDData(&operatorID);
    operatorID.OperatorIdType = ODID_OPERATOR_ID;
    char operatorId[] = "98765432100123456789";
    strncpy(operatorID.OperatorId, operatorId, sizeof(operatorID.OperatorId));
    encodeOperatorIDMessage(&OperatorID_enc, &operatorID);
    // 清空数组，将所有元素设置为 0
    memset(service_OperatorID_data, 0, sizeof(service_OperatorID_data));
    // 固定前5个元素
    service_OperatorID_data[0] = 0x1E;
    service_OperatorID_data[1] = 0x16;
    service_OperatorID_data[2] = 0xFA;
    service_OperatorID_data[3] = 0xFF;
    service_OperatorID_data[4] = 0x0D;
    // 让 service_data[3] 自增到 0xFF 后清零
    static uint8_t counter = 0;
    service_OperatorID_data[5] = counter++;
    if (counter == 0xFF) {
        counter = 0;
    }
    // 计算正确的复制长度
    size_t OperatorID_len = sizeof(OperatorID_enc);
    memcpy(&service_OperatorID_data[6], (uint8_t*) &OperatorID_enc, OperatorID_len);
    //
    #if ble_rid_info
        for (size_t i = 0; i < sizeof(service_OperatorID_data); i++) {
            printf("%02X ", service_OperatorID_data[i]);
        }
    #endif
    esp_err_t ret;
    ret = esp_ble_gap_ext_adv_stop(1,&ext_adv[2]);  // 停止当前广播
    if (ret) {
        printf("stop adv data failed, error code = %x", ret);
    }
    // config adv data
    ret = esp_ble_gap_config_ext_adv_data_raw(2, sizeof(service_OperatorID_data), &service_OperatorID_data[0]);
    if (ret) {
        printf("config adv data failed, error code = %x", ret);
    }
    ret = esp_ble_gap_ext_adv_start(1,&ext_adv[2]);  // 重启广播
    if (ret) {
        printf("start adv data failed, error code = %x", ret);
    }
}
static void fill_example_data(void)
{
    if (ODID_AUTH_MAX_PAGES < 2) {
        fprintf(stderr, "Program compiled with ODID_AUTH_MAX_PAGES < 2\n");
        return;
    }

    odid_initBasicIDData(&BasicID);
    BasicID.IDType = ODID_IDTYPE_CAA_REGISTRATION_ID;
    BasicID.UAType = ODID_UATYPE_HELICOPTER_OR_MULTIROTOR;
    char id[] = "12345678901234567890";
    strncpy(BasicID.UASID, id, sizeof(BasicID.UASID));
    encodeBasicIDMessage(&BasicID_enc, &BasicID);

    if (xSemaphoreTake(gps_Mutex, portMAX_DELAY) == pdTRUE) {
        // 访问共享资源
        //printf("\n----------update_wifi_gps_data-----\n");
        odid_initLocationData(&Location);
        Location.Status = ODID_STATUS_AIRBORNE;
        Location.Direction = 215.7f;
        Location.SpeedHorizontal = 5.4f;
        Location.SpeedVertical = 5.25f;
        Location.Latitude = 45.539309;
        Location.Longitude = -122.966389;
        Location.AltitudeBaro = 100;
        Location.AltitudeGeo = 110;
        Location.HeightType = ODID_HEIGHT_REF_OVER_GROUND;
        Location.Height = 80;
        Location.HorizAccuracy = createEnumHorizontalAccuracy(2.5f);
        Location.VertAccuracy = createEnumVerticalAccuracy(0.5f);
        Location.BaroAccuracy = createEnumVerticalAccuracy(1.5f);
        Location.SpeedAccuracy = createEnumSpeedAccuracy(0.5f);
        Location.TSAccuracy = createEnumTimestampAccuracy(0.2f);
        Location.TimeStamp = 360.52f;       
        xSemaphoreGive(gps_Mutex);  // 释放互斥信号量
    }
    encodeLocationMessage(&Location_enc, &Location);

    odid_initAuthData(&Auth0);
    Auth0.AuthType = ODID_AUTH_UAS_ID_SIGNATURE;
    Auth0.DataPage = 0;
    Auth0.LastPageIndex = 1;
    Auth0.Length = 40;
    Auth0.Timestamp = 28000000;
    char auth0_data[] = "12345678901234567";
    memcpy(Auth0.AuthData, auth0_data, MINIMUM(sizeof(auth0_data), sizeof(Auth0.AuthData)));
    encodeAuthMessage(&Auth0_enc, &Auth0);

    odid_initAuthData(&Auth1);
    Auth1.AuthType = ODID_AUTH_UAS_ID_SIGNATURE;
    Auth1.DataPage = 1;
    char auth1_data[] = "12345678901234567890123";
    memcpy(Auth1.AuthData, auth1_data, MINIMUM(sizeof(auth1_data), sizeof(Auth1.AuthData)));
    encodeAuthMessage(&Auth1_enc, &Auth1);

    odid_initSelfIDData(&SelfID);
    SelfID.DescType = ODID_DESC_TYPE_TEXT;
    char description[] = "DronesRUS: Real Estate";
    strncpy(SelfID.Desc, description, sizeof(SelfID.Desc));
    encodeSelfIDMessage(&SelfID_enc, &SelfID);

    odid_initSystemData(&System_data);
    System_data.OperatorLocationType = ODID_OPERATOR_LOCATION_TYPE_TAKEOFF;
    System_data.ClassificationType = ODID_CLASSIFICATION_TYPE_EU;
    System_data.OperatorLatitude = Location.Latitude + 0.00001;
    System_data.OperatorLongitude = Location.Longitude + 0.00001;
    System_data.AreaCount = 35;
    System_data.AreaRadius = 75;
    System_data.AreaCeiling = 176.9f;
    System_data.AreaFloor = 41.7f;
    System_data.CategoryEU = ODID_CATEGORY_EU_SPECIFIC;
    System_data.ClassEU = ODID_CLASS_EU_CLASS_3;
    System_data.OperatorAltitudeGeo = 20.5f;
    System_data.Timestamp = 28000000;
    encodeSystemMessage(&System_enc, &System_data);

    odid_initOperatorIDData(&operatorID);
    operatorID.OperatorIdType = ODID_OPERATOR_ID;
    char operatorId[] = "98765432100123456789";
    strncpy(operatorID.OperatorId, operatorId, sizeof(operatorID.OperatorId));
    encodeOperatorIDMessage(&OperatorID_enc, &operatorID);

    odid_initMessagePackData(&pack);
    pack.MsgPackSize = 7;
    memcpy(&pack.Messages[0], &BasicID_enc, ODID_MESSAGE_SIZE);
    memcpy(&pack.Messages[1], &Location_enc, ODID_MESSAGE_SIZE);
    memcpy(&pack.Messages[2], &Auth0_enc, ODID_MESSAGE_SIZE);
    memcpy(&pack.Messages[3], &Auth1_enc, ODID_MESSAGE_SIZE);
    memcpy(&pack.Messages[4], &SelfID_enc, ODID_MESSAGE_SIZE);
    memcpy(&pack.Messages[5], &System_enc, ODID_MESSAGE_SIZE);
    memcpy(&pack.Messages[6], &OperatorID_enc, ODID_MESSAGE_SIZE);
    encodeMessagePack(&pack_enc, &pack);
    #if ble_rid_info
        printf("----------pack_enc: -------\n");
        printByteArray((uint8_t*) &pack_enc, ODID_MESSAGE_SIZE*7+3, 1);
    #endif
    uint8_t *pack_data = (uint8_t*)&pack_enc;
    // 计算要拷贝的数据长度（从pack_enc的第3字节开始）
    const uint16_t enc_data_len = ODID_MESSAGE_SIZE*pack.MsgPackSize+3; // 总长
    // 固定头部配置
    service_REMOTEID_data[0] = 5 + enc_data_len; // 总有效数据长度（前5字节 + 数据段）
    service_REMOTEID_data[1] = 0x16;
    service_REMOTEID_data[2] = 0xFA;
    service_REMOTEID_data[3] = 0xFF;
    service_REMOTEID_data[4] = 0x0D;
    // 自增计数器处理
    static uint8_t counter = 0;
    service_REMOTEID_data[5] = counter++;
    if (counter == 0xFF) { // 溢出归零
        counter = 0;
    }
    // 拷贝数据：从pack_enc的第三个字节开始（跳过前2字节）
    memcpy(&service_REMOTEID_data[6], pack_data, enc_data_len);
    #if ble_rid_info
        printf("----------service data : -------\n");
        for (size_t i = 0; i < service_REMOTEID_data[0]; i++) {
            printf("%02X ", service_REMOTEID_data[i]);
            if ((i + 1) % 25 == 0) printf("\n");
        }
    #endif

    FUNC_SEND_WAIT_SEM(esp_ble_gap_ext_adv_stop(1,&ext_adv[3]),set_sem);  // 停止当前广播
    // config adv data
    FUNC_SEND_WAIT_SEM(esp_ble_gap_config_ext_adv_data_raw(3, service_REMOTEID_data[0]+1, &service_REMOTEID_data[0]),set_sem);
    FUNC_SEND_WAIT_SEM(esp_ble_gap_ext_adv_start(1,&ext_adv[3]),set_sem);  // 重启广播     
}
uint8_t ble_enable;
/* 在app_main函数前添加任务实现 */
static void ble_adv_task(void *pvParameters)
{
    static int count = 0;
    while (1) {
        if(ble_enable)
        {   
            count++;
            if(count == 1)
            {
                fill_BasicID_encoded();
            }      
            else if(count == 2)
            {
                fill_Location_encoded();
            }
            else if(count == 3)
            {
                fill_AuthData0_encoded();
            }
            else if(count == 4)
            {
                fill_AuthData1_encoded();
            }  
            else if(count == 5)
            {
                fill_SelfID_encoded();
            }  
            else if(count == 6)
            {
                fill_System_encoded();
            } 
            else if(count == 7)
            {
                fill_OperatorID_encoded();
                count = 0;
            }     
        }
        vTaskDelay(pdMS_TO_TICKS(200));  // ms
    }
}

//蓝牙ble拓展5.0广播
static void ble_ext_adv_task(void *pvParameters)
{
    while (1) 
    {
        if(ble_enable)
        { 
            fill_example_data();
            vTaskDelay(pdMS_TO_TICKS(500));  // ms
        }
    }
}

int ble_init(void)
{    
    esp_err_t ret;
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(LOG_TAG, "%s initialize controller failed: %s", __func__, esp_err_to_name(ret));
        return -1;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(LOG_TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
        return -1;
    }

    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(LOG_TAG, "%s init bluetooth failed: %s", __func__, esp_err_to_name(ret));
        return -1;
    }
    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(LOG_TAG, "%s enable bluetooth failed: %s", __func__, esp_err_to_name(ret));
        return -1;
    }
    ret = esp_ble_gap_register_callback(gap_event_handler);
    if (ret){
        ESP_LOGE(LOG_TAG, "gap register error, error code = %x", ret);
        return -1;
    }
    // 注册 GATT 回调函数，处理所有的 GATT 事件
    ret = esp_ble_gatts_register_callback(gatts_event_handler);
    if (ret){
        ESP_LOGE(LOG_TAG, "gatts register error, error code = %x", ret);
        return -1;
    }
   
    esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
    if (local_mtu_ret){
        ESP_LOGE(LOG_TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
        return -1;
    }
    vTaskDelay(200 / portTICK_PERIOD_MS);

    set_sem = xSemaphoreCreateBinary();
    // 1M phy extend adv, Connectable advertising
    // FUNC_SEND_WAIT_SEM(esp_ble_gap_ext_adv_set_params(0, &ext_adv_params_1M), set_sem);
    // FUNC_SEND_WAIT_SEM(esp_ble_gap_ext_adv_set_rand_addr(0, addr_1m), set_sem);
    // FUNC_SEND_WAIT_SEM(esp_ble_gap_config_ext_adv_data_raw(0, sizeof(raw_adv_data_1m), &raw_adv_data_1m[0]), set_sem);

    // // 2M phy extend adv, Scannable advertising
    // FUNC_SEND_WAIT_SEM(esp_ble_gap_ext_adv_set_params(1, &ext_adv_params_2M), set_sem);
    // FUNC_SEND_WAIT_SEM(esp_ble_gap_ext_adv_set_rand_addr(1, addr_2m), set_sem);
    // FUNC_SEND_WAIT_SEM(esp_ble_gap_config_ext_scan_rsp_data_raw(1, sizeof(raw_scan_rsp_data_2m), raw_scan_rsp_data_2m), set_sem);

    // 1M phy legacy adv, ADV_IND
    FUNC_SEND_WAIT_SEM(esp_ble_gap_ext_adv_set_params(2, &legacy_adv_params), set_sem);
    FUNC_SEND_WAIT_SEM(esp_ble_gap_ext_adv_set_rand_addr(2, addr_legacy), set_sem);
    FUNC_SEND_WAIT_SEM(esp_ble_gap_config_ext_adv_data_raw(2, sizeof(legacy_adv_data), &legacy_adv_data[0]), set_sem);
    FUNC_SEND_WAIT_SEM(esp_ble_gap_config_ext_scan_rsp_data_raw(2, sizeof(legacy_scan_rsp_data), &legacy_scan_rsp_data[0]), set_sem);

    // coded phy extend adv, Scannable advertising
    FUNC_SEND_WAIT_SEM(esp_ble_gap_ext_adv_set_params(3, &ext_adv_params_coded), set_sem);
    FUNC_SEND_WAIT_SEM(esp_ble_gap_ext_adv_set_rand_addr(3, addr_coded), set_sem);
    FUNC_SEND_WAIT_SEM(esp_ble_gap_config_ext_scan_rsp_data_raw(3, sizeof(raw_scan_rsp_data_coded), &raw_scan_rsp_data_coded[0]), set_sem);

    // start all adv
    FUNC_SEND_WAIT_SEM(esp_ble_gap_ext_adv_start(2, &ext_adv[2]), set_sem);

    ble_enable = 1;
    BaseType_t xReturn;
    xReturn = xTaskCreatePinnedToCore(ble_adv_task,"ble_adv_task",8192,NULL,14,NULL, tskNO_AFFINITY);
    if(xReturn != pdPASS) 
    {
        printf("xTaskCreatePinnedToCore ble_adv_task error!\r\n");
    }
    xReturn = xTaskCreatePinnedToCore(ble_ext_adv_task,"ble_ext_adv_task",8192,NULL,14,NULL, tskNO_AFFINITY);
    if(xReturn != pdPASS) 
    {
        printf("xTaskCreatePinnedToCore ble_ext_adv_task error!\r\n");
    }
    return 1;
}

