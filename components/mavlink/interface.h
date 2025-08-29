#ifndef _INTERFACE_H_
#define _INTERFACE_H_
#include "driver/uart.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mavlink.h"
#include "esp_log.h"
#include "string.h"

typedef struct Mavlink_Messages 
{
  int sysid;
  int compid;

  mavlink_message_t msg;
  // Heartbeat
  mavlink_heartbeat_t heartbeat;
  // System Status
  mavlink_sys_status_t sys_status;
  // Battery Status
  mavlink_battery_status_t battery_status;
  // Global Position
  mavlink_global_position_int_t global_position_int;
}Mavlink_Messages;

typedef struct _Interface
{
  int system_id;
  int autopilot_id;
  int companion_id;
}_Interface;

void mavlink_Interface_init(void);

#endif