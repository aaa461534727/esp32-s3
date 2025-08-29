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

#include "lwip/err.h"
#include "lwip/sys.h"

#include "gps.h"
#include "opendroneid.h"
#include "wifi_ap.h"
#include "ble_peripheral.h"
#include "web_server.h"
#include "power.h"
#include "../components/mavlink/interface.h"
#include "global.h"

SemaphoreHandle_t gps_Mutex = NULL;

void app_main(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    //创建gps互斥信号量
    gps_Mutex = xSemaphoreCreateMutex();  // 创建互斥信号量
    if (gps_Mutex != NULL) {
        printf("Success to create mutex\n"); 
    } else {
        printf("Failed to create mutex\n");
    }
    //初始化外设
    power_gpio_init();
    wifi_init();
    ble_init();
    start_web_server();  // 启动Web服务器
    gps_Interface_init();
    mavlink_Interface_init();
}
