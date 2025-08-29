#include "power.h"

void gpio_output_init(gpio_num_t gpio_num) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << gpio_num), // 指定 GPIO 引脚（如 GPIO_NUM_2）
        .mode = GPIO_MODE_OUTPUT,           // 设置为输出模式
        .pull_up_en = GPIO_PULLUP_DISABLE,  // 禁用上拉电阻
        .pull_down_en = GPIO_PULLDOWN_DISABLE, // 禁用下拉电阻
        .intr_type = GPIO_INTR_DISABLE      // 关闭中断
    };
    gpio_config(&io_conf);
}

void gpio_input_init(gpio_num_t gpio_num) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << gpio_num), // 指定 GPIO 引脚（如 GPIO_NUM_4）
        .mode = GPIO_MODE_INPUT,            // 设置为输入模式
        .pull_up_en = GPIO_PULLUP_ENABLE,   // 启用上拉电阻（默认高电平）
        .pull_down_en = GPIO_PULLDOWN_DISABLE, // 禁用下拉电阻
        .intr_type = GPIO_INTR_DISABLE      // 关闭中断
    };
    gpio_config(&io_conf);
}

void power_gpio_init(void)
{
    
}