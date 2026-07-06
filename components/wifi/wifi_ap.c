#include "wifi_ap.h"
#include "rid_parse_gb.h"

#define EXAMPLE_ESP_WIFI_SSID      "ESP32_REMOTE_ID"
#define EXAMPLE_ESP_WIFI_PASS      "QQQ123456789"
#define EXAMPLE_ESP_WIFI_CHANNEL   6
#define EXAMPLE_MAX_STA_CONN       4

static const char *TAG = "wifi softAP";


static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
}

void wifi_init_softap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // 创建默认AP网络接口
    esp_netif_t *softap_netif = esp_netif_create_default_wifi_ap();
    
    // 设置静态IP
    esp_netif_ip_info_t ip_info;
    ip_info.ip.addr = ipaddr_addr("192.168.1.1");
    ip_info.netmask.addr = ipaddr_addr("255.255.255.0");
    ip_info.gw.addr = ipaddr_addr("192.168.1.1");
    
    // 停止并重置DHCP
    esp_netif_dhcps_stop(softap_netif);
    esp_netif_set_ip_info(softap_netif, &ip_info);
    esp_netif_dhcps_start(softap_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            .channel = EXAMPLE_ESP_WIFI_CHANNEL,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .max_connection = EXAMPLE_MAX_STA_CONN,
#ifdef CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT
            .authmode = WIFI_AUTH_WPA3_PSK,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
#else /* CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT */
            .authmode = WIFI_AUTH_WPA2_PSK,
#endif
            .pmf_cfg = {
                    .required = true,
            },
        },
    };
    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS, EXAMPLE_ESP_WIFI_CHANNEL);
    
}
//定义rid信息
static ODID_BasicID_encoded BasicID_enc;
static ODID_BasicID_data BasicID;
//static ODID_BasicID_data BasicID_out;

static ODID_Location_encoded Location_enc;
static ODID_Location_data Location;
//static ODID_Location_data Location_out;

static ODID_Auth_encoded Auth0_enc;
static ODID_Auth_encoded Auth1_enc;
static ODID_Auth_data Auth0;
static ODID_Auth_data Auth1;
// static ODID_Auth_data Auth0_out;
// static ODID_Auth_data Auth1_out;

static ODID_SelfID_encoded SelfID_enc;
static ODID_SelfID_data SelfID;
//static ODID_SelfID_data SelfID_out;

static ODID_System_encoded System_enc;
static ODID_System_data System_data;
//static ODID_System_data System_out;

static ODID_OperatorID_encoded OperatorID_enc;
static ODID_OperatorID_data operatorID;
//static ODID_OperatorID_data operatorID_out;

static ODID_MessagePack_encoded pack_enc;
static ODID_MessagePack_data pack;
static ODID_UAS_Data uasData;

int length;
uint8_t send_counter_beacon=0;
uint8_t send_counter_nan=0;
uint8_t buffer[1024]={0};
uint8_t wifi_ap_mac[6] = {0};

#define  wifi_rid_info  1

void fill_example_data()
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
    decodeMessagePack(&uasData, &pack_enc);
    #if wifi_rid_info 
        printf("\n-------------------------------------Encoded Data-----------------------------------\n");
        printf("            0- 1- 2- 3- 4- 5- 6- 7- 8- 9- 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24\n");
        printf("BasicID:    ");
        printByteArray((uint8_t*) &BasicID_enc, ODID_MESSAGE_SIZE, 1);

        printf("Location:   ");
        printByteArray((uint8_t*) &Location_enc, ODID_MESSAGE_SIZE, 1);

        printf("Auth0:      ");
        printByteArray((uint8_t*) &Auth0_enc, ODID_MESSAGE_SIZE, 1);

        printf("Auth1:      ");
        printByteArray((uint8_t*) &Auth1_enc, ODID_MESSAGE_SIZE, 1);

        printf("SelfID:     ");
        printByteArray((uint8_t*) &SelfID_enc, ODID_MESSAGE_SIZE, 1);

        printf("System:     ");
        printByteArray((uint8_t*) &System_enc, ODID_MESSAGE_SIZE, 1);

        printf("OperatorID: ");
        printByteArray((uint8_t*) &OperatorID_enc, ODID_MESSAGE_SIZE, 1);

        printf("----------pack_enc: -------\n");
        printByteArray((uint8_t*) &pack_enc, ODID_MESSAGE_SIZE*7+3, 1);
    #endif 
    //获取热点地址
    if(esp_read_mac(wifi_ap_mac,ESP_MAC_WIFI_SOFTAP) == ESP_OK )
    {
        #if wifi_rid_info 
            printf("AP MAC: %02X:%02X:%02X:%02X:%02X:%02X", 
                wifi_ap_mac[0], wifi_ap_mac[1], wifi_ap_mac[2], 
                wifi_ap_mac[3], wifi_ap_mac[4], wifi_ap_mac[5]);
        #endif 
    }
    if ((length = odid_wifi_build_message_pack_beacon_frame(&uasData,(char *)wifi_ap_mac,
    "UAS_ID_OPEN", strlen("UAS_ID_OPEN"), //use dummy SSID, as we only extract payload data
   100, ++send_counter_beacon, buffer, sizeof(buffer))) > 0)
   {
        //set the RID IE element
        uint8_t header_offset = 58;
        // 1. 计算payload的偏移量
        const size_t payload_offset = offsetof(vendor_ie_data_t, payload);

        // 2. 动态分配内存（关键：柔性数组需要额外空间）
        size_t total_ie_size = sizeof(vendor_ie_data_t) + (length - header_offset);
        vendor_ie_data_t *IE_data = (vendor_ie_data_t*)malloc(total_ie_size);
        if (!IE_data) {
            printf("Memory allocation failed!\n");
            return;
        }
        memset(IE_data, 0, sizeof(vendor_ie_data_t)); // 仅清空头部

        // 3. 填充结构体头部
        IE_data->element_id = WIFI_VENDOR_IE_ELEMENT_ID;
        IE_data->vendor_oui[0] = 0xFA;
        IE_data->vendor_oui[1] = 0x0B;
        IE_data->vendor_oui[2] = 0xBC;
        IE_data->vendor_oui_type = 0x0D;
        IE_data->length = (length - header_offset) + 4; // 总长度 = 数据长度 + 头部长度（element_id + length + vendor_oui + type）

        // 4. 拷贝数据到柔性数组区域
        memcpy((uint8_t*)IE_data + payload_offset, &buffer[header_offset], length - header_offset);

        // // 5. 打印原始Buffer数据
        // printf("\n--------1----- -\n");
        // for (size_t i = 0; i < length - header_offset; i++) {
        //     printf("%02X ", buffer[header_offset + i]);
        //     if ((i + 1) % 25 == 0) printf("\n");
        // }

        // // 6. 打印结构体中的实际数据
        // printf("\n--------2----- -\n");
        // uint8_t *payload_ptr = (uint8_t*)IE_data + payload_offset;
        // for (size_t i = 0; i < length - header_offset; i++) {
        //     printf("%02X ", payload_ptr[i]);
        //     if ((i + 1) % 25 == 0) printf("\n");
        // }

        // 7. 设置Vendor IE（注意：需确保IE_data内存连续）
        if (esp_wifi_set_vendor_ie(false, WIFI_VND_IE_TYPE_BEACON, WIFI_VND_IE_ID_0, IE_data) != ESP_OK) {
            printf("Failed to remove old IE\n");
        }
        if (esp_wifi_set_vendor_ie(true, WIFI_VND_IE_TYPE_BEACON, WIFI_VND_IE_ID_0, IE_data) != ESP_OK) {
            printf("Failed to add new IE\n");
        }

        // 8. 释放内存
        free(IE_data);
    }
}

/* 新国标 GB 46750-2025 Beacon 发射 */
static void fill_gb_beacon(void)
{
    struct rid_info gb;
    memset(&gb, 0, sizeof(gb));

    /* === 全部写死数据 === */
    gb.lat = 45.539310f;
    gb.lon = -122.966389f;
    gb.speed = 5.4f;
    gb.direction = 215.7f;
    gb.alt = 100.0f;        /* 相对高度 */
    gb.geo_high = 110.0f;   /* 大地高度 */
    gb.air_high = 80.0f;    /* 气压高度 */
    gb.v_speed = 5.25f;     /* 垂直速度 */
    gb.ilot_lat_gb = 45.539320f;
    gb.ilot_lon_gb = -122.966375f;
    gb.ilot_height = 10.0f;
    gb.status = 2;          /* 空中 */
    gb.gb_category = 1;     /* 开放类 */
    gb.gb_class = 0;        /* 微型 */
    gb.ilot_loc_type = 0;   /* 起飞点 */
    gb.coord_system = 0;    /* WGS-84 */
    gb.hor_accuracy = 2;
    gb.ver_accuracy = 2;
    gb.speed_accuracy = 2;
    gb.unix_ts_ms = 0;
    gb.ts_accuracy = 0;
    strncpy(gb.upi, "1986EXD30260602AHT12", 20);
    strncpy(gb.reg_id, "12345678", 8);

    uint8_t gb_tlv[256];
    int tlv_len = rid_gb_encode(gb_tlv, sizeof(gb_tlv), &gb);
    printf("GB: tlv_len=%d\n", tlv_len);
    if (tlv_len <= 0) return;

    /* 自己构建 Beacon 帧 (200字节够用) */
    uint8_t buf[256];
    int len = 0;
    const uint8_t bcast[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
    const char *ssid = "ESP32_REMOTE_ID";
    int ssid_len = strlen(ssid);
    uint8_t oui[3] = {0xFA, 0x0B, 0xBC};
    uint8_t oui_type = 0x0D;

    /* mgmt header (24B) */
    memset(buf + len, 0, 24);
    buf[len] = 0x80; buf[len+1] = 0x00; /* frame_control = beacon */
    memcpy(buf+len+4, bcast, 6);   /* da */
    memcpy(buf+len+10, wifi_ap_mac, 6); /* sa */
    memcpy(buf+len+16, wifi_ap_mac, 6); /* bssid */
    len += 24;

    /* beacon body (12B) */
    memset(buf + len, 0, 12);
    buf[len+8] = 0xC8; buf[len+9] = 0x00; /* beacon_interval = 200 TU */
    buf[len+10] = 0x21; buf[len+11] = 0x04; /* capability */
    len += 12;

    /* SSID IE */
    buf[len] = 0x00; buf[len+1] = (uint8_t)ssid_len;
    memcpy(buf+len+2, ssid, ssid_len);
    len += 2 + ssid_len;

    /* Supported Rates IE */
    buf[len] = 0x01; buf[len+1] = 0x01; buf[len+2] = 0x8C;
    len += 3;

    /* Vendor IE: element_id(0xDD) + length + OUI(3) + type(1) + counter(1) + 0xFF(1) + TLV */
    int payload_len = 1 + tlv_len; /* counter + TLV */
    int vendor_data_len = 4 + payload_len; /* OUI(3)+type(1)+payload */

    buf[len] = 0xDD;                    /* element_id */
    buf[len+1] = (uint8_t)vendor_data_len;
    buf[len+2] = oui[0]; buf[len+3] = oui[1]; buf[len+4] = oui[2];
    buf[len+5] = oui_type;
    buf[len+6] = 0x00;               /* message_counter = 0 (固定) */
    /* TLV 数据直接从 rid_gb_encode 输出拷贝 (不需要额外加 0xFF) */
    memcpy(buf+len+7, gb_tlv, tlv_len);
    len += 6 + payload_len;

    printf("GB: tx_len=%d tlv=%d\n", len, tlv_len);
    esp_err_t ret = esp_wifi_80211_tx(WIFI_IF_AP, buf, len, true);
    if (ret != ESP_OK) {
        printf("GB: tx FAILED ret=%d\n", ret);
    } else {
        printf("GB: tx OK\n");
    }
}

static void wifi_beacon_task(void *pvParameters)
{
    while (1) {
        fill_example_data();
        vTaskDelay(pdMS_TO_TICKS(150));
        fill_gb_beacon();
        vTaskDelay(pdMS_TO_TICKS(150));
    }
}

void wifi_send_init(void)
{
    wifi_init_softap();
    BaseType_t xReturn ;
    //接收gps数据
    xReturn = xTaskCreatePinnedToCore(wifi_beacon_task,"wifi_beacon_task",10240,NULL,13,NULL, tskNO_AFFINITY);
    if(xReturn != pdPASS) 
    {
        printf("xTaskCreatePinnedToCore wifi_beacon_task error!\r\n");
    }
}