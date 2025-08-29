#ifndef _WIFI_AP_H_
#define _WIFI_AP_H_

#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_timer.h"

#include "lwip/ip4_addr.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/inet.h"
#include "lwip/lwip_napt.h"

#include "sdkconfig.h"
#include "global.h"
#include "../libopendroneid/opendroneid.h"
extern void udp_client_init(void); 
void wifi_init(void);
#endif