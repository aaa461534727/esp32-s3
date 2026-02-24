/*  WiFi softAP Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_spiffs.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#include "gps.h"
#include "opendroneid.h"
#include "rid_wifi_sniffer.h"
#include "ble.h"
#include "ble_peripheral.h"
#include "web_server.h"
#include "power.h"
#include "cpu_info.h"
#include "ec200x.h"
#include "udp_client.h"
#include "iot.h"
#include "cloudupdate.h"
#include "../components/mavlink/interface.h"
#include "global.h"

static const char *TAG = "main";
SemaphoreHandle_t gps_Mutex = NULL;

static void mount_spiffs(void) {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = "storage",
        .max_files = 5,
        .format_if_mount_failed = true
    };
    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount SPIFFS (%s)", esp_err_to_name(ret));
        return;
    }
    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "SPIFFS: total=%d, used=%d", total, used);
    }
}

void app_main(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    mount_spiffs();
    
    //创建gps互斥信号量
    // gps_Mutex = xSemaphoreCreateMutex();  // 创建互斥信号量
    // if (gps_Mutex != NULL) {
    //     printf("Success to create mutex\n"); 
    // } else {
    //     printf("Failed to create mutex\n");
    // }
    //初始化外设
    // power_gpio_init();
    // wifi_init();
    // ble_init();
    // ble_sniffer_init();
    
    // gps_Interface_init();//GPS
    // mavlink_Interface_init();//无人机协议
    // cpu_info_init();
    ec200x_init();
    rid_wifi_sniffer_init();
    udp_client_init();
    iot_init();
    // start_cloud_update();
    start_web_server();  // 启动Web服务器
}
