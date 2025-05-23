/*
 * 2024 Guangshi (Shanghai) CO LTD
 *
 * Wing Hu
 */

#include "ai_gpio.h"
#include "driver/gpio.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

void ai_gpio_init(void) {
    /*---- GPIO输出模式配置 ----*/
    // 初始化GPIO配置结构体
    gpio_config_t io_conf = {};
    // 禁用中断功能
    io_conf.intr_type = GPIO_INTR_DISABLE;
    // 设置为输出模式
    io_conf.mode = GPIO_MODE_OUTPUT;
    // 通过位掩码选择要配置的GPIO引脚
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    // 关闭下拉电阻
    io_conf.pull_down_en = 0;
    // 关闭上拉电阻
    io_conf.pull_up_en = 0;
    // 应用GPIO配置
    gpio_config(&io_conf);

    /*---- 传感器硬件初始化序列 ----*/
    // 拉低复位引脚（硬件复位准备）
    gpio_set_level(GPIO_OUTPUT_IO_RST, 0);


    // 保持低电平100ms确保复位有效
    vTaskDelay(100 / portTICK_PERIOD_MS);

    // 释放复位引脚（结束硬件复位）
    gpio_set_level(GPIO_OUTPUT_IO_RST, 1);
    // 等待传感器初始化完成
    vTaskDelay(100 / portTICK_PERIOD_MS);
}