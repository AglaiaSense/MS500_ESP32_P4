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

#include "esp_sleep.h"

static const char *TAG = "APP_MAIN";

// 全局变量声明
device_ctx_t *device_ctx;
bool storage_enabled = true; // 添加全局标志

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

    uvc_device_deinit();

    bsp_uvc_deinit(device_ctx);

    bsp_video_deinit(device_ctx);

    // sd卡最后卸载，别的功能还在用sd卡
    bsp_deinit_sd_card(device_ctx);

    ESP_LOGI(TAG, "enter_light_sleep_before ----------------------------------------- ");

    delay_ms(500); // 延时1秒
    
}
void enter_light_sleep_after() {
    ESP_LOGI(TAG, "enter_light_sleep_after ----------------------------------------- ");

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

        // if (count > 100) {
        //     if (count % 10 == 0) {
        //         storage_enabled = !storage_enabled;
        //         ESP_LOGI(TAG, "Storage %s", storage_enabled ? "Enabled" : "Disabled");
        //     }
        // }
        if (count % 15 == 0) {

            enter_light_sleep_before();

            // enter_light_sleep_time(SEC_TO_USEC(10));
            enter_light_sleep_gpio();

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

    // 确保以下电源域在睡眠时保持开启
    // esp_sleep_pd_config(ESP_PD_DOMAIN_VDDSDIO, ESP_PD_OPTION_ON);
    // esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON); // RTC 外设
    // esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_ON);       // 主晶振
    // ESP_LOGE(TAG, "VDDSDIO : %d", esp_sleep_pd_config(ESP_PD_DOMAIN_VDDSDIO, ESP_PD_OPTION_AUTO));

    inin_spiffs();
    init_device_ctx();

    bsp_init_sd_card(device_ctx);

    bsp_video_init(device_ctx);

    bsp_uvc_init(device_ctx);
    usb_cam2_init();
    uvc_device_init();


    ESP_LOGI(TAG, "finalizing ----------------------------------------- ");
}
