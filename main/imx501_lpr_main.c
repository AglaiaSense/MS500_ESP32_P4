/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "ai_gpio.h"
#include "cJSON.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "fw_dnn.h"
#include "fw_loader.h"
// #include "i2c_tool.h"
#include "ai_spi_dev.h"
#include "ai_spi_receive.h"
#include "esp_spiffs.h"
#include "imx501.h"
#include "sdkconfig.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "ai_i2c.h"
#include "bsp_spiff.h"

/* SPI_BOOT, FLASH_BOOT, FLASH_UPDATE智能三者选择其一 */
#define SPI_BOOT (0)
#define FLASH_BOOT (1)
#define FLASH_UPDATE (0)

char car_reader[12];
static const char *TAG = "HTTP_CLIENT";

char warter_mutex = 1;
char lp_detected = 0;
int car_mutex = 1;

int car_count = 0;

// 定义事件组句柄
EventGroupHandle_t boot_event_group;
// 定义 LTE 启动成功的事件标志
#define LTE_BOOT_OK_BIT (1 << 0)
#define UDP_BOOT_OK_BIT (1 << 1)

void http_get_task(void *pvParameters) {

    while (true) {

        // 等待 LTE 启动成功的事件
        EventBits_t uxBits = xEventGroupWaitBits(
            boot_event_group,                  // 事件组句柄
            LTE_BOOT_OK_BIT | UDP_BOOT_OK_BIT, // 等待的事件标志
            pdFALSE,                           // 不清除事件标志
            pdFALSE,                           // 不等待所有标志位
            portMAX_DELAY                      // 无限期等待
        );

        if ((uxBits & (LTE_BOOT_OK_BIT | UDP_BOOT_OK_BIT)) == (LTE_BOOT_OK_BIT | UDP_BOOT_OK_BIT)) {
            // 每1秒获取一次水表数据
            printf("%s(%d)\n\r", __func__, __LINE__);
            vTaskDelay(pdMS_TO_TICKS(1000));

            // 连续多次检查到
            // if (lp_detected == 1) {
            //     lp_detected = 0;
            //     car_count++;
            //     printf("car_count: %d\n", car_count);

            //     if (car_count >= 3) {
            //         car_mutex = 0;
            //         // send_get_request();
            //         send_post_request();
            //         car_mutex = 1;
            //     }

            // } else {
            //     vTaskDelay(pdMS_TO_TICKS(1000));
            // }
        }
    }
}

/* 主函数 */
void app_main(void) {

    // ------------------------ 初始化硬件 ------------------------
    ai_gpio_init();

    bsp_spiff_init();

    spi_master_dev_init();

    ai_i2c_master_init();

    //   while (1) {
    //     // vTaskDelay(pdMS_TO_TICKS(50));
    //     printf("app_main\n");
    //     spi_master_test();
    // }

    // ------------------------ 读写IMX500模型和寄存器 ------------------------

    /* 初始化imx501 */
    imx501_init();

    /* firmware初始化 */
    fw_init();

#if SPI_BOOT
    fw_spi_boot();
    dnn_spi_boot();
#endif

#if FLASH_BOOT
    fw_flash_boot();
    printf("%s(%d) \n", __func__, __LINE__);

    dnn_flash_boot();
    printf("%s(%d) \n", __func__, __LINE__);

#endif

#if FLASH_UPDATE
    fw_flash_update();
    dnn_flash_update();
#endif

    /* fimware反初始化 */
    fw_uninit();

    // ------------------------ 读写IMX500模型和寄存器 ------------------------

    /* 使能视频检查输出 */
    stream_start();

    /* 销毁spi master，然后初始化spi slave，他们共同使用SPI2 */
    spi_master_dev_destroy();
    spi_slave_dev_init();

    /* spi slave接收数据并处理 */
    spi_slave_receive_data();
}
