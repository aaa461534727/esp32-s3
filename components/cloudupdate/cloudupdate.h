#ifndef __CLUODUPDATE_H__
#define __CLUODUPDATE_H__
#include "esp_log.h"
#include "string.h"
#include "driver/gpio.h"
#include "global.h"
#include "../4G/ec200x.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "cJSON.h"
#include "esp_mac.h"
#include "esp_heap_caps.h"
#include "esp_flash.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_flash_partitions.h"
#include "cloudupdate.h"
#include <string.h>
#include <sys/time.h>

void start_cloud_update(void);

#endif