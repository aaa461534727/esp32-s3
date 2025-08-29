#include "interface.h"

#define TXD2_PIN (GPIO_NUM_6)
#define RXD2_PIN (GPIO_NUM_7)
static const int RX_BUF_SIZE = 1024;
static mavlink_status_t mavlink_status; // 全局或静态状态机

static void uart2_init(void) {
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    // We won't use a buffer for sending data.
    uart_driver_install(UART_NUM_2, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_2, &uart_config);
    uart_set_pin(UART_NUM_2, TXD2_PIN, RXD2_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

void rx2_task(void *arg)
{
    static const char *RX_TASK_TAG = "RX2_TASK";
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    uint8_t* data = (uint8_t*) malloc(RX_BUF_SIZE+1);
    while (1) {
        const int rxBytes = uart_read_bytes(UART_NUM_2, data, RX_BUF_SIZE, 10 / portTICK_PERIOD_MS);
         //printf("\n----------RX2- %d-----\n",rxBytes);
        if (rxBytes > 0) {
            data[rxBytes] = 0;
            ESP_LOGI(RX_TASK_TAG, "Read %d bytes: '%s'", rxBytes, data);
            ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, data, rxBytes, ESP_LOG_INFO);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    free(data);
}
/*
// ------------------------------------------------------------------------------
//   Read Messages
// ------------------------------------------------------------------------------
*/

void read_messages(struct Mavlink_Messages *current_messages) 
{ 
  
  uint8_t byte;
  int len = uart_read_bytes(UART_NUM_2, &byte, 1, 100 / portTICK_PERIOD_MS);

  if(len>0) 
  {
    //ESP_LOGI("RX_TASK_TAG", "Received byte: 0x%02X", byte);
    // Try to get a new message
    if(mavlink_parse_char(MAVLINK_COMM_0, byte, &(current_messages->msg), &mavlink_status)) 
    {
        // Note this doesn't handle multiple message sources.
        current_messages->sysid = current_messages->msg.sysid;
        current_messages->compid = current_messages->msg.compid;

        // 打印消息头信息
        printf("\n=== MAVLink Message Received ===\n");
        printf("System ID: %d\n", current_messages->msg.sysid);
        printf("Component ID: %d\n", current_messages->msg.compid);
        printf("Message ID: %d (0x%04X)\n", current_messages->msg.msgid, current_messages->msg.msgid);
        printf("Sequence: %d\n", current_messages->msg.seq);
        printf("Payload Length: %d\n", current_messages->msg.len);
        printf("Checksum: 0x%04X\n", current_messages->msg.checksum);

        // Handle Message ID
        switch (current_messages->msg.msgid)
        {   
            //FD 09 00 00 4C 42 01 00 00 00 04 00 00 00 02 0C 80 04 03 2C 8D
            case MAVLINK_MSG_ID_HEARTBEAT:
            {
                mavlink_msg_heartbeat_decode(&current_messages->msg, &(current_messages->heartbeat));	
                printf("\n[HEARTBEAT]\n");
                printf("Type: %d (%s)\n", current_messages->heartbeat.type, 
                    (current_messages->heartbeat.type == MAV_TYPE_QUADROTOR) ? "Quadrotor" : "Unknown");
                printf("Autopilot: %d (%s)\n", current_messages->heartbeat.autopilot,
                    (current_messages->heartbeat.autopilot == MAV_AUTOPILOT_PX4) ? "PX4" : "Other");
                printf("Base Mode: 0x%02X\n", current_messages->heartbeat.base_mode);
                printf("Custom Mode: 0x%08lX\n", current_messages->heartbeat.custom_mode);
                printf("System Status: %d (%s)\n", current_messages->heartbeat.system_status,
                    (current_messages->heartbeat.system_status == MAV_STATE_ACTIVE) ? "Active" : "Inactive");
                printf("MAVLink Version: %d\n", current_messages->heartbeat.mavlink_version);
                break;
            }
            //FD 1F 00 00 07 42 4D 01 00 00 FF FF 00 00 FF FF 00 00 FF FF 00 00 F4 01 E0 2E 64 00 00 00 00 00 00 00 00 00 00 00 00 00 50 C6 7D 
            case MAVLINK_MSG_ID_SYS_STATUS:
            {
                mavlink_msg_sys_status_decode(&current_messages->msg, &(current_messages->sys_status));
                printf("\n[SYS_STATUS]\n");
                printf("Voltage: %.2f V\n", current_messages->sys_status.voltage_battery / 1000.0f);
                printf("Current: %.2f A\n", current_messages->sys_status.current_battery / 100.0f);
                printf("Battery Remaining: %d%%\n", current_messages->sys_status.battery_remaining);
                printf("Load: %.1f%%\n", current_messages->sys_status.load / 10.0f);
                printf("Errors (Comm: %d, Power: %d)\n", 
                    current_messages->sys_status.errors_comm, current_messages->sys_status.errors_count1);
                break;
            }
            //FD 29 00 00 10 42 4D 93 00 00 DC 05 00 00 4C 1D 00 00 C4 09 68 10 36 10 FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF F4 01 00 01 03 50 00 00 00 00 01 38 54
            case MAVLINK_MSG_ID_BATTERY_STATUS:
            {
                mavlink_msg_battery_status_decode(&current_messages->msg, &(current_messages->battery_status));
                printf("\n[BATTERY_STATUS]\n");
                printf("Battery ID: %d\n", current_messages->battery_status.id);
                printf("Remaining: %d%%\n", current_messages->battery_status.battery_remaining);
                printf("Temperature: %.1f C\n", current_messages->battery_status.temperature / 100.0f);
                // 打印各电芯电压
                for(int i=0; i<10; i++) {
                    if(current_messages->battery_status.voltages[i] == UINT16_MAX) continue;
                    printf("Cell %d: %.3f V\n", i+1, current_messages->battery_status.voltages[i] / 1000.0f);
                }
                break;
            }
            //FD 1C 00 00 0A 42 4D 21 00 00 40 E2 01 00 24 D2 6F 0D 80 DB 05 44 A0 86 01 00 88 13 00 00 2C 01 38 FF 00 00 28 23 B4 40
            case MAVLINK_MSG_ID_GLOBAL_POSITION_INT:
            {
                mavlink_msg_global_position_int_decode(&current_messages->msg, &(current_messages->global_position_int));
                printf("\n[GLOBAL_POSITION]\n");
                printf("Latitude: %.7f°\n", current_messages->global_position_int.lat / 1e7);
                printf("Longitude: %.7f°\n", current_messages->global_position_int.lon / 1e7);
                printf("Altitude (MSL): %.2f m\n", current_messages->global_position_int.alt / 1000.0f);
                printf("Altitude (AGL): %.2f m\n", current_messages->global_position_int.relative_alt / 1000.0f);
                printf("Heading: %.2f°\n", current_messages->global_position_int.hdg / 100.0f);
                break;
            }

            default:
            {
                printf("\n[UNHANDLED MESSAGE]\n");
                printf("Message ID %d not implemented\n", current_messages->msg.msgid);
                break;
            }
        }
    
    }
      
  }

}

// ------------------------------------------------------------------------------
//  Set Stream Data  
// ------------------------------------------------------------------------------

int set_message_interval(struct _Interface Interface)
{
    // //心跳包
    // mavlink_message_t heartbeat_msg;
    // mavlink_msg_heartbeat_pack(
    //     Interface.system_id,      // 本机系统ID
    //     MAV_COMP_ID_AUTOPILOT1,   // 本机组件ID
    //     &heartbeat_msg,
    //     MAV_TYPE_QUADROTOR,       // 载具类型
    //     MAV_AUTOPILOT_PX4,        // 飞控类型
    //     MAV_MODE_FLAG_SAFETY_ARMED,  // 系统模式
    //     4,                        // 自定义模式（如定高）
    //     MAV_STATE_ACTIVE          // 系统状态
    // );
  
    // //系统状态信息
    // mavlink_message_t sys_msg;
    // uint8_t system_status = MAV_STATE_ACTIVE;
    // uint32_t sensor_flags = 0xFFFF; // 假设所有传感器正常
    // // 打包SYS_STATUS消息
    // mavlink_msg_sys_status_pack(
    //     Interface.system_id, Interface.companion_id, &sys_msg,
    //     sensor_flags,      // onboard_control_sensors_present
    //     sensor_flags,      // onboard_control_sensors_enabled
    //     sensor_flags,      // onboard_control_sensors_health
    //     500,               // load (500%表示50% CPU负载)
    //     12000,             // voltage_battery (12V)
    //     100,               // current_battery (1A)
    //     80,                // battery_remaining (80%)
    //     0,                 // drop_rate_comm (0%)
    //     0,                 // errors_comm
    //     0, 0, 0, 0,         // errors_count1-4
    //     0,  // onboard_control_sensors_present_extended
    //     0,  // onboard_control_sensors_enabled_extended
    //     0   // onboard_control_sensors_health_extended
    // );

    // //电池信息
    // mavlink_message_t battery_msg;
    // // 示例：发送电池状态（假设扩展字段未使用）
    // uint16_t voltages[10] = {4200, 4150, UINT16_MAX, UINT16_MAX, UINT16_MAX, 
    //     UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX};
    // uint16_t voltages_ext[4] = {0};  // 扩展电压数组（未使用则填0）
    // uint8_t mode = 0;                // 默认模式
    // uint32_t fault_bitmask = 0;      // 无故障
    // mavlink_msg_battery_status_pack(
    //     Interface.system_id,Interface.companion_id, &battery_msg,
    //     0,                          // 电池ID
    //     MAV_BATTERY_FUNCTION_ALL,    // 电池功能
    //     MAV_BATTERY_TYPE_LION,       // 电池类型
    //     2500,                       // 温度（25.00°C）
    //     voltages,                    // 电压数组
    //     500,                        // 电流（5.0A）
    //     1500,                       // 消耗电量（1500mAh）
    //     7500,                       // 消耗能量（7500J）
    //     80,                         // 剩余电量（80%）
    //     0,                          // 剩余时间（未知）
    //     MAV_BATTERY_CHARGE_STATE_OK, // 充电状态
    //     voltages_ext,               // 扩展电压数组
    //     mode,                       // 电池模式
    //     fault_bitmask               // 故障位掩码
    // );

    //位置信息
    mavlink_message_t position_msg;
    // 示例调用（系统ID=1，组件ID=0）
    mavlink_msg_global_position_int_pack(
        Interface.system_id,Interface.companion_id, &position_msg,
        123456,             // 示例时间戳
        225432100,          // 纬度22.543210°
        1141234560,         // 经度114.123456°
        100000,             // 海拔高度100米
        5000,               // 相对高度5米
        300,                // 北向速度3 m/s → 300 cm/s
        -200,               // 东向速度-2 m/s → -200 cm/s
        0,                  // 垂直速度0 cm/s
        9000                // 航向90度 → 9000厘弧度
    );
    
    char buf[MAVLINK_MAX_PACKET_LEN];
    unsigned len = mavlink_msg_to_send_buffer((uint8_t*)buf, &position_msg);
    uart_write_bytes(UART_NUM_2, (const char *)buf, (uint8_t)len);
    return len;
}

static void test_send_task(void *arg)
{
    _Interface Interface;
    Interface.system_id = 66;
    Interface.companion_id = 77;
    while (1)
    {
        set_message_interval(Interface);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void test_rec_task(void *arg)
{
    Mavlink_Messages current_messages;
    while (1)
    {
        read_messages(&current_messages);
        vTaskDelay(1);
    }
}

void mavlink_Interface_init(void)
{
    uart2_init();
    mavlink_status_t *status = mavlink_get_channel_status(MAVLINK_COMM_0);
    status->flags &= ~MAVLINK_STATUS_FLAG_IN_MAVLINK1; // 清除V1标志

    BaseType_t xReturn ;
    xReturn = xTaskCreatePinnedToCore(test_rec_task,"test_rec_task",8192,NULL,13,NULL, tskNO_AFFINITY);
    if(xReturn != pdPASS) 
    {
        printf("xTaskCreatePinnedToCore test_rec_task error!\r\n");
    }

    /*创建发送任务*///改创建任务的函数可以执行CPU核心0、1，当前参数不指定
    xReturn = xTaskCreatePinnedToCore(test_send_task,"test_send_task",4096,NULL,12,NULL, tskNO_AFFINITY);
    if(xReturn != pdPASS) 
    {
        printf("xTaskCreatePinnedToCore test_send_task error!\r\n");
    }

}

