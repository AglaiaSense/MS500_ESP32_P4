/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: ESPRESSIF MIT
 */

#include "esp_spiffs.h"

#include "bsp_config.h"
#include "bsp_rtc.h"
#include "bsp_sd_card.h"
#include "bsp_sleep.h"
#include "bsp_uvc_cam.h"
#include "bsp_uvc_jpg.h"
#include "bsp_video.h"
#include "bsp_video_jpg.h"

#include "ai_gpio.h"
#include "esp_sleep.h"

#include "imx501.h"

#include "ai_gpio.h"
#include "ai_i2c.h"
#include "ai_spi_dev.h"
#include "ai_spi_receive.h"
#include "fw_dnn.h"
#include "fw_loader.h"

#include "bsp_spiff.h"

/* SPI_BOOT, FLASH_BOOT, FLASH_UPDATE智能三者选择其一 */
#define SPI_BOOT (0)
#define FLASH_BOOT (1)
#define FLASH_UPDATE (0)

char car_reader[12];
char lp_detected = 0;
int car_mutex = 1;

static const char *TAG = "APP_MAIN";

// 全局变量声明
device_ctx_t *device_ctx;

// 是否存图片
bool is_store_jpg_allow = false; // 是否允许存储图片
bool is_store_jpg_doing = false; // 存储未结束，直接睡眠会闪退，所以要加个标志位

// 初始化 uvc_t 结构体
void init_device_ctx(void) {
    device_ctx = calloc(1, sizeof(device_ctx_t));
    if (device_ctx == NULL) {
        // 处理内存分配失败的情况
        ESP_LOGE("BSP_CONFIG", "Failed to allocate memory for uvc_t");
    }
}

void enter_light_sleep_before() {

    // 优化图片存储
    is_store_jpg_allow = false; // 禁止存储图片
    while (is_store_jpg_doing == true) {
        delay_ms(500);
    }
    printf("is_store_jpg_doing = %d\n", is_store_jpg_doing);

    uvc_device_deinit();

    bsp_uvc_deinit(device_ctx);

    bsp_video_deinit(device_ctx);

    // sd卡最后卸载，别的功能还在用sd卡
    bsp_deinit_sd_card(device_ctx);

    ESP_LOGI(TAG, "-----------------------------------------：enter_light_sleep_before  ");

    delay_ms(500); // 延时1秒
}
void enter_light_sleep_after() {
    ESP_LOGI(TAG, "-----------------------------------------： enter_light_sleep_after");
    // is_store_jpg_allow = true;

    delay_ms(500); // 延时1秒

    bsp_init_sd_card(device_ctx);
    bsp_test_read();

    bsp_video_init(device_ctx);

    bsp_uvc_init(device_ctx);
    usb_cam2_init();
    uvc_device_init();
}

// 定义周期性任务
void periodic_task(void *pvParameters) {
    int count = 0;
    while (1) {
        printf("weak task running...: %d\r\n", count++);
        vTaskDelay(pdMS_TO_TICKS(1000)); // 延时1秒

        if (count % 10 == 0) {
            bsp_video_jpg_init(device_ctx);
        }
        if (count % 30 == 0) {

            //     enter_light_sleep_before();

            //     enter_light_sleep_time(SEC_TO_USEC(10));
            //     // enter_light_sleep_gpio();

            //     enter_light_sleep_after();
        }
    }
}
void video_cam_init(void) {

    // 这里初始化I2c,mipi
    bsp_video_init(device_ctx);

    // bsp_uvc_init(device_ctx);
    // usb_cam2_init();
    // uvc_device_init();

    // bsp_video_jpg_init(device_ctx);

    ESP_LOGI(TAG, "finalizing ----------------------------------------- ");
}

void imx501_lpr_main(void) {

    // ------------------------ 读写IMX500模型和寄存器 ------------------------
    printf("%s(%d) \n", __func__, __LINE__);
    /* 初始化imx501 */
    imx501_init();
    printf("%s(%d) \n", __func__, __LINE__);

    /* firmware初始化 */
    fw_init();
    printf("%s(%d) \n", __func__, __LINE__);

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
    printf("%s(%d) \n", __func__, __LINE__);

    // ------------------------ 读写IMX500模型和寄存器 ------------------------
    printf("%s(%d) \n", __func__, __LINE__);

    /* 使能视频检查输出 */
    stream_start();
    printf("%s(%d) \n", __func__, __LINE__);

    /* 销毁spi master，然后初始化spi slave，他们共同使用SPI2 */
    spi_master_dev_destroy();
    spi_slave_dev_init();
    printf("%s(%d) \n", __func__, __LINE__);

}
void imx500_lpr_receive(void) {

    printf("%s(%d) \n", __func__, __LINE__);

    /* spi slave接收数据并处理 */
    spi_slave_receive_data();
}
void app_device_init(void) {
    printf(" __  __  ____  _____  _____ \n");
    printf("|  \\/  |/ ___||  _  ||  _  |\n");
    printf("| .  . |\\___ \\| | | || | | |\n");
    printf("| |\\/| | ___) | |_| || |_| |\n");
    printf("|_|  |_||____/ \\___/  \\___/ \n");

    xTaskCreate(periodic_task, "periodic_task", 1024 * 8, NULL, 5, NULL);
    ESP_LOGI(TAG, "Initializing ----------------------------------------- ");

    // ------------------------ 初始化硬件 ------------------------

    ai_gpio_init();

    bsp_spiff_init();

    bsp_init_sd_card(device_ctx);

    spi_master_dev_init();

    // 在video中会单独初始化
    // ai_i2c_master_init();
}
void app_config_init(void) {
    init_device_ctx();
}

void app_main(void) {

    app_config_init();

    app_device_init();

    video_cam_init();
    printf("%s(%d) \n", __func__, __LINE__);

    // MIPI里面有I2C
    ai_i2c_copy_sccb_handle();


    imx501_lpr_main();

    imx500_lpr_receive();
}
