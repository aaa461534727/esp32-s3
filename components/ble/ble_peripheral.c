#include "ble_peripheral.h"
#include "../wifi/rid_parse.h"
#include "../wifi/rid_parse_gb.h"

#define LOG_TAG "BLE_ADV"
#define GB_BLE_BUF_SIZE 256

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
    .secondary_phy = ESP_BLE_GAP_PHY_2M,
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
    ret = esp_ble_gap_ext_adv_stop(1, (const uint8_t[]){2});  // 停止当前广播
    if (ret) {
        printf("stop adv data failed, error code = %x", ret);
    }
    // config adv data
    ret = esp_ble_gap_config_ext_adv_data_raw(2, sizeof(service_BasicID_data), &service_BasicID_data[0]);
    if (ret) {
        printf("config adv data failed, error code = %x", ret);
    }
    ret = esp_ble_gap_ext_adv_start(1, &ext_adv[2]);  // 重启广播
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
    ret = esp_ble_gap_ext_adv_stop(1, (const uint8_t[]){2});  // 停止当前广播
    if (ret) {
        printf("stop adv data failed, error code = %x", ret);
    }
    // config adv data
    ret = esp_ble_gap_config_ext_adv_data_raw(2, sizeof(service_Location_data), &service_Location_data[0]);
    if (ret) {
        printf("config adv data failed, error code = %x", ret);
    }
    ret = esp_ble_gap_ext_adv_start(1, &ext_adv[2]);  // 重启广播
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
    ret = esp_ble_gap_ext_adv_stop(1, (const uint8_t[]){2});  // 停止当前广播
    if (ret) {
        printf("stop adv data failed, error code = %x", ret);
    }
    // config adv data
    ret = esp_ble_gap_config_ext_adv_data_raw(2, sizeof(service_Auth0_data), &service_Auth0_data[0]);
    if (ret) {
        printf("config adv data failed, error code = %x", ret);
    }
    ret = esp_ble_gap_ext_adv_start(1, &ext_adv[2]);  // 重启广播
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
    ret = esp_ble_gap_ext_adv_stop(1, (const uint8_t[]){2});  // 停止当前广播
    if (ret) {
        printf("stop adv data failed, error code = %x", ret);
    }
    // config adv data
    ret = esp_ble_gap_config_ext_adv_data_raw(2, sizeof(service_Auth1_data), &service_Auth1_data[0]);
    if (ret) {
        printf("config adv data failed, error code = %x", ret);
    }
    ret = esp_ble_gap_ext_adv_start(1, &ext_adv[2]);  // 重启广播
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
    ret = esp_ble_gap_ext_adv_stop(1, (const uint8_t[]){2});  // 停止当前广播
    if (ret) {
        printf("stop adv data failed, error code = %x", ret);
    }
    // config adv data
    ret = esp_ble_gap_config_ext_adv_data_raw(2, sizeof(service_SelfID_data), &service_SelfID_data[0]);
    if (ret) {
        printf("config adv data failed, error code = %x", ret);
    }
    ret = esp_ble_gap_ext_adv_start(1, &ext_adv[2]);  // 重启广播
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
    ret = esp_ble_gap_ext_adv_stop(1, (const uint8_t[]){2});  // 停止当前广播
    if (ret) {
        printf("stop adv data failed, error code = %x", ret);
    }
    // config adv data
    ret = esp_ble_gap_config_ext_adv_data_raw(2, sizeof(service_System_data), &service_System_data[0]);
    if (ret) {
        printf("config adv data failed, error code = %x", ret);
    }
    ret = esp_ble_gap_ext_adv_start(1, &ext_adv[2]);  // 重启广播
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
    ret = esp_ble_gap_ext_adv_stop(1, (const uint8_t[]){2});  // 停止当前广播
    if (ret) {
        printf("stop adv data failed, error code = %x", ret);
    }
    // config adv data
    ret = esp_ble_gap_config_ext_adv_data_raw(2, sizeof(service_OperatorID_data), &service_OperatorID_data[0]);
    if (ret) {
        printf("config adv data failed, error code = %x", ret);
    }
    ret = esp_ble_gap_ext_adv_start(1, &ext_adv[2]);  // 重启广播
    if (ret) {
        printf("start adv data failed, error code = %x", ret);
    }
}
/*
 * fill_gb_data — 发送 GB 46750-2025 新国标 RID 数据
 * 替代 fill_example_data，用 rid_gb_encode() 编码
 * BLE AD 格式: [len, 0x16, 0xFA, 0xFF, 0x0D, counter, gb_tlv...]
 */
static uint8_t service_GB_data[GB_BLE_BUF_SIZE] = {0};
static void fill_gb_data(void)
{
    struct rid_info gb_info;
    memset(&gb_info, 0, sizeof(gb_info));

    /* Field 001: UPI — 20 字节 ASCII */
    strcpy(gb_info.upi, "123456BLE50123AHT33");
    /* Field 002: reg_id — 8 字节 ASCII */
    strcpy(gb_info.reg_id, "BLE12345");
    /* Field 003: 无人机类别 */
    gb_info.gb_category = 2;  /* 2 = 多旋翼 */
    /* Field 004: 无人机等级 */
    gb_info.gb_class = 1;     /* 1 = C1 */
    /* Field 005: 操作员位置类型 */
    gb_info.ilot_loc_type = 0; /* 0 = 起飞点 */

    /* Field 006: 遥控站位置 — 从 GPS 数据取（有锁就用 GPS，否则用固定值） */
    if (xSemaphoreTake(gps_Mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        if (gpsData.fix_status) {
            gb_info.ilot_lon_gb = (float)gpsData.longitude;
            gb_info.ilot_lat_gb = (float)gpsData.latitude;
        } else {
            gb_info.ilot_lon_gb = 121.4737f; /* 上海 */
            gb_info.ilot_lat_gb = 31.2304f;
        }
        xSemaphoreGive(gps_Mutex);
    } else {
        gb_info.ilot_lon_gb = 121.4737f;
        gb_info.ilot_lat_gb = 31.2304f;
    }
    /* Field 007: 遥控站高度 */
    gb_info.ilot_height = 50.0f;

    /* Field 008: UA 位置 */
    gb_info.lon = 121.4740f;
    gb_info.lat = 31.2310f;
    /* Field 009: 航迹角 */
    gb_info.direction = 90.0f;
    /* Field 010: 地速 */
    gb_info.speed = 5.4f;
    /* Field 011: 相对高度 */
    gb_info.alt = 80.0f;
    /* Field 012: 垂直速度 */
    gb_info.v_speed = 0.5f;
    /* Field 013: 大地高度 */
    gb_info.geo_high = 85.0f;
    /* Field 014: 气压高度 */
    gb_info.air_high = 80.0f;

    /* Field 015: 运行状态 */
    gb_info.status = 1;       /* 1 = airborne */
    /* Field 016: 坐标系类型 */
    gb_info.coord_system = 0; /* 0 = WGS-84 */
    /* Field 017-019: 精度 */
    gb_info.hor_accuracy = 2;
    gb_info.ver_accuracy = 2;
    gb_info.speed_accuracy = 2;
    /* Field 020: 时间戳 — 自动填充 (gettimeofday) */
    gb_info.unix_ts_ms = 0;   /* rid_gb_encode 里自动获取 */
    /* Field 021: 时间戳精度 */
    gb_info.ts_accuracy = 0;

    /* 调用 rid_gb_encode 编码 */
    unsigned char gb_tlv[200];
    int tlv_len = rid_gb_encode(gb_tlv, sizeof(gb_tlv), &gb_info);
    if (tlv_len <= 0) {
        ESP_LOGE(LOG_TAG, "rid_gb_encode failed, len=%d", tlv_len);
        return;
    }

    /* 构造 BLE AD 包: [len, 0x16, 0xFA, 0xFF, 0x0D, counter, gb_tlv...] */
    /* AD structure = 1(len) + 1(type) + 2(UUID) + 1(ODID) + 1(counter) + tlv_len */
    int payload_len = 1 + 2 + 1 + 1 + tlv_len; /* type(1B) + UUID(2B) + ODID(1B) + counter(1B) + TLV */
    int total_len = 1 + payload_len;            /* + AD length byte */

    if (total_len > GB_BLE_BUF_SIZE || total_len > 190) {
        ESP_LOGE(LOG_TAG, "GB data too large: %d > 190 BLE ExtAdv limit", total_len);
        return;
    }

    memset(service_GB_data, 0, sizeof(service_GB_data));
    service_GB_data[0] = (uint8_t)payload_len;  /* AD length */
    service_GB_data[1] = 0x16;                   /* AD type: Service Data 16-bit UUID */
    service_GB_data[2] = 0xFA;                   /* UUID low byte */
    service_GB_data[3] = 0xFF;                   /* UUID high byte (0xFFFA = ASTM RID) */
    service_GB_data[4] = 0x0D;                   /* ODID message type */
    /* counter */
    static uint8_t gb_counter = 0;
    service_GB_data[5] = gb_counter++;
    if (gb_counter == 0xFF) gb_counter = 0;
    /* GB TLV payload */
    memcpy(&service_GB_data[6], gb_tlv, tlv_len);

    ESP_LOGI(LOG_TAG, "GB BLE: tlv_len=%d total=%d counter=%d", tlv_len, total_len, service_GB_data[5]);

    /* 更新 BLE 广播数据 (instance 3 = CODED PHY) */
    FUNC_SEND_WAIT_SEM(esp_ble_gap_ext_adv_stop(1, (const uint8_t[]){3}), set_sem);
    FUNC_SEND_WAIT_SEM(esp_ble_gap_config_ext_adv_data_raw(3, total_len, &service_GB_data[0]), set_sem);
    FUNC_SEND_WAIT_SEM(esp_ble_gap_ext_adv_start(1, &ext_adv[3]), set_sem);
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

//蓝牙ble拓展5.0广播 — 发射 GB 46750 新国标 RID 数据
static void ble_ext_adv_task(void *pvParameters)
{
    while (1) 
    {
        if(ble_enable)
        { 
            fill_gb_data();
            vTaskDelay(pdMS_TO_TICKS(1000));  /* 1s 间隔 */
        }
    }
}

int ble_send_init(void)
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
    FUNC_SEND_WAIT_SEM(esp_ble_gap_ext_adv_set_params(0, &ext_adv_params_1M), set_sem);
    FUNC_SEND_WAIT_SEM(esp_ble_gap_ext_adv_set_rand_addr(0, addr_1m), set_sem);
    FUNC_SEND_WAIT_SEM(esp_ble_gap_config_ext_adv_data_raw(0, sizeof(raw_adv_data_1m), &raw_adv_data_1m[0]), set_sem);

    // // 2M phy extend adv, Scannable advertising
    FUNC_SEND_WAIT_SEM(esp_ble_gap_ext_adv_set_params(1, &ext_adv_params_2M), set_sem);
    FUNC_SEND_WAIT_SEM(esp_ble_gap_ext_adv_set_rand_addr(1, addr_2m), set_sem);
    FUNC_SEND_WAIT_SEM(esp_ble_gap_config_ext_scan_rsp_data_raw(1, sizeof(raw_scan_rsp_data_2m), raw_scan_rsp_data_2m), set_sem);

    // 1M phy legacy adv, ADV_IND
    FUNC_SEND_WAIT_SEM(esp_ble_gap_ext_adv_set_params(2, &legacy_adv_params), set_sem);
    FUNC_SEND_WAIT_SEM(esp_ble_gap_ext_adv_set_rand_addr(2, addr_legacy), set_sem);
    FUNC_SEND_WAIT_SEM(esp_ble_gap_config_ext_adv_data_raw(2, sizeof(legacy_adv_data), &legacy_adv_data[0]), set_sem);
    FUNC_SEND_WAIT_SEM(esp_ble_gap_config_ext_scan_rsp_data_raw(2, sizeof(legacy_scan_rsp_data), &legacy_scan_rsp_data[0]), set_sem);

    // coded phy extend adv, Scannable advertising
    FUNC_SEND_WAIT_SEM(esp_ble_gap_ext_adv_set_params(3, &ext_adv_params_coded), set_sem);
    FUNC_SEND_WAIT_SEM(esp_ble_gap_ext_adv_set_rand_addr(3, addr_coded), set_sem);
    FUNC_SEND_WAIT_SEM(esp_ble_gap_config_ext_scan_rsp_data_raw(3, sizeof(raw_scan_rsp_data_coded), &raw_scan_rsp_data_coded[0]), set_sem);

    // 启动全部4个扩展广播实例（0、1、2、3）
    FUNC_SEND_WAIT_SEM(esp_ble_gap_ext_adv_start(4, ext_adv), set_sem);

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

