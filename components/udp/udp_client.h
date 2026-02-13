#ifndef UDP_CLIENT_H
#define UDP_CLIENT_H

#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "esp_log.h"

#include "lwip/sockets.h"
#include "lwip/inet.h"
#include <stdbool.h>
#include <stdint.h>
#include "esp_log.h"
#include "string.h"
#include "driver/gpio.h"
#include "global.h"
#include "../4G/ec200x.h"

void udp_client_init(void);
void udp_client_free(void);
bool udp_client_send(const void *data, size_t len, TickType_t wait_ticks);
#endif // UDP_CLIENT_H