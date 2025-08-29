#ifndef _GPS_H_
#define _GPS_H_
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"
#include "global.h"

void gps_Interface_init(void);
#endif