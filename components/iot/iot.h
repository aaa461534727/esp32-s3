#ifndef _IOT_H_
#define _IOT_H_
#include "esp_log.h"
#include "string.h"
#include "driver/gpio.h"
#include "global.h"
#include "../4G/ec200x.h"
#include "esp_app_desc.h"      // esp_app_desc_t / esp_app_get_description
#include "esp_idf_version.h"   // ESP_IDF_VERSION_xxx
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "esp_timer.h"

#include "mqtt_client.h"
#include "cJSON.h"

#include "iot_interaction.h"

void iot_init(void);
void free_iot_init(void);
#endif