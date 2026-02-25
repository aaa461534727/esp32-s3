#ifndef _WEB_SERVER_H_
#define _WEB_SERVER_H_
#include "global.h"
#include "rid_wifi_sniffer.h"
#include "iot.h"
#include "udp_client.h"
bool is_ota_in_progress(void);
void start_web_server(void);

#endif