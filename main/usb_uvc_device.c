/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include "esp_log.h"
#include "usb_device_uvc.h"
#include "usb_cam.h"
#include "esp_spiffs.h"
 


// 添加FreeRTOS相关头文件
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// 定义周期性任务
void periodic_task(void *pvParameters)
{
    int count = 0;
    while (1) {
        printf("weak task running...: %d\r\n", count++);
        vTaskDelay(pdMS_TO_TICKS(1000)); // 延时1秒
    }
}


void app_main(void)
{
  
    printf(" _   _ _   _ _____     _____ _____ _____ _____ \n");
    printf("| | | | | | /  __ \\   |_   _|  ___/  ___|_   _|\n");
    printf("| | | | | | | /  \\/_____| | | |__ \\ `--.  | |  \n");
    printf("| | | | | | | |  |______| | |  __| `--. \\ | |\n");
    printf("| |_| \\ \\_/ / \\__/\\     | | | |___/\\__/ / | |\n");
    printf(" \\___/ \\___/ \\____/     \\_/ \\____/\\____/  \\_/\n");
    printf("USB Device UVC Test\n");    
    // 创建周期性任务
    xTaskCreate(periodic_task, "periodic_task", 2048, NULL, 5, NULL);

    fflush(stdout);

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = "avi",
        .max_files = 5,   // This decides the maximum number of files that can be created on the storage
        .format_if_mount_failed = false
    };

    ESP_ERROR_CHECK(esp_vfs_spiffs_register(&conf));

    ESP_ERROR_CHECK(usb_cam1_init());
#if CONFIG_UVC_SUPPORT_TWO_CAM
    ESP_ERROR_CHECK(usb_cam2_init());
#endif
    ESP_ERROR_CHECK(uvc_device_init());



}
