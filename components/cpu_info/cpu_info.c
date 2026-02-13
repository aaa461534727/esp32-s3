#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_heap_caps.h"

void monitor_task(void *pvParameter) {
    char task_list_buffer[512];
    char cpu_stats_buffer[400];
    
    while(1) {
        // 1. 打印任务列表信息
        memset(task_list_buffer, 0, sizeof(task_list_buffer));
        vTaskList((char *)&task_list_buffer);
        printf("\n任务名\t状态\t优先级\t剩余栈\t序号\tCPU核\n");
        printf("******************************************************\n");
        printf("%s\n", task_list_buffer);
        
        // 2. 打印各任务CPU占用率
        memset(cpu_stats_buffer, 0, sizeof(cpu_stats_buffer));
        vTaskGetRunTimeStats((char *)&cpu_stats_buffer);
        printf("\n任务名\t\t运行计数\t使用率\n");
        printf("=====================================================\n");
        printf("%s\n", cpu_stats_buffer);
        
        // 3. 打印内存使用情况
        printf("\n内存信息:\n");
        printf("  当前空闲堆内存: %ld 字节\n", esp_get_free_heap_size());
        printf("  内部RAM空闲: %d 字节\n", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
        printf("  SPIRAM空闲: %d 字节\n", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
        
        // 4. 打印当前任务的栈高水位线（可选）
        printf("  本监控任务栈最小剩余: %d 字节\n", uxTaskGetStackHighWaterMark(NULL));
        
        printf("\n\n");
        vTaskDelay(pdMS_TO_TICKS(5000)); // 每5秒打印一次
    }
}

// 创建监控任务
void cpu_info_init(void) {
    xTaskCreate(monitor_task, "sys_monitor", 4096, NULL, 1, NULL);
}