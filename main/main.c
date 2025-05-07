/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: ESPRESSIF MIT
 */

#include <stdio.h>

#include "bsp_config.h"
#include "bsp_sd_card.h"
#include "bsp_uvc_cam.h"
#include "bsp_uvc_jpg.h"
#include "bsp_video.h"

#include "esp_spiffs.h"

static const char *TAG = "APP_MAIN";

// 全局变量声明
device_ctx_t *device_ctx;
bool storage_enabled = false;  // 添加全局标志

// 初始化 uvc_t 结构体
void bsp_struct_alloc(void) {
    device_ctx = calloc(1, sizeof(device_ctx_t));
    if (device_ctx == NULL) {
        // 处理内存分配失败的情况
        ESP_LOGE("BSP_CONFIG", "Failed to allocate memory for uvc_t");
    }
}

// 定义周期性任务
void periodic_task(void *pvParameters)
{
    int count = 0;
    while (1) {
        printf("weak task running...: %d\r\n", count++);
        vTaskDelay(pdMS_TO_TICKS(1000)); // 延时1秒

        if (count % 10 == 0) {
            storage_enabled = !storage_enabled;
            ESP_LOGI(TAG, "Storage %s", storage_enabled ? "Enabled" : "Disabled");
        }

    }
}

void app_main(void) {
    printf(" __  __  ____  _____  _____ \n");
    printf("|  \\/  |/ ___||  _  ||  _  |\n");
    printf("| .  . |\\___ \\| | | || | | |\n");
    printf("| |\\/| | ___) | |_| || |_| |\n");
    printf("|_|  |_||____/ \\___/  \\___/ \n");

    xTaskCreate(periodic_task, "periodic_task", 2048, NULL, 5, NULL);
    ESP_LOGI(TAG, "Initializing ----------------------------------------- ");
    

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = "avi",
        .max_files = 5, // This decides the maximum number of files that can be created on the storage
        .format_if_mount_failed = false};

    ESP_ERROR_CHECK(esp_vfs_spiffs_register(&conf));
    

    bsp_struct_alloc();
    bsp_video_init(device_ctx);
    bsp_init_sd_card(device_ctx);

    bsp_uvc_init(device_ctx);

#if CONFIG_UVC_SUPPORT_TWO_CAM
    ESP_ERROR_CHECK(usb_cam2_init());
#endif

ESP_ERROR_CHECK(uvc_device_init());


    bsp_sd_card_test(device_ctx);

    ESP_LOGI(TAG, "finalizing ----------------------------------------- ");
}
