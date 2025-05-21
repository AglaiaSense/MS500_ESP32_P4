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

#include "esp_sleep.h"
#include "ai_gpio.h"

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
void inin_spiffs(void) {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = "avi",
        .max_files = 5, // This decides the maximum number of files that can be created on the storage
        .format_if_mount_failed = false};

    ESP_ERROR_CHECK(esp_vfs_spiffs_register(&conf));
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

        if (count % 5 == 0) {
            //   bsp_video_jpg_init(device_ctx);

        }
        if (count % 30 == 0) {

            enter_light_sleep_before();

            enter_light_sleep_time(SEC_TO_USEC(10));
            // enter_light_sleep_gpio();

            enter_light_sleep_after();
        }
    }
}

void app_main(void) {
    printf(" __  __  ____  _____  _____ \n");
    printf("|  \\/  |/ ___||  _  ||  _  |\n");
    printf("| .  . |\\___ \\| | | || | | |\n");
    printf("| |\\/| | ___) | |_| || |_| |\n");
    printf("|_|  |_||____/ \\___/  \\___/ \n");

    xTaskCreate(periodic_task, "periodic_task", 1024 * 8, NULL, 5, NULL);
    ESP_LOGI(TAG, "Initializing ----------------------------------------- ");
    ai_gpio_init();

  
    inin_spiffs();
    init_device_ctx();

    bsp_init_sd_card(device_ctx);

    bsp_video_init(device_ctx);

    bsp_uvc_init(device_ctx);
    usb_cam2_init();
    uvc_device_init();

    bsp_video_jpg_init(device_ctx);

    ESP_LOGI(TAG, "finalizing ----------------------------------------- ");
}
