#ifndef _EC200X_H_
#define _EC200X_H_
#include "esp_log.h"
#include "string.h"
#include "driver/gpio.h"
#include "global.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_netif_ppp.h"
#include "esp_timer.h"

#include "driver/uart.h"

#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/inet.h"
#include "rid_wifi_sniffer.h"
#include "iot.h"
#include "udp_client.h"

bool ec200x_is_network_connected(void);
BaseType_t ec200x_wait_for_network(TickType_t timeout_ticks);
void get_ppp_ipv4_gw_mask(char *ip, size_t ip_len, char *mask, size_t mask_len, char *gw, size_t gw_len);
void ec200x_init(void);
#endif