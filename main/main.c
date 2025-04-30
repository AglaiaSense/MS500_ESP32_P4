/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: ESPRESSIF MIT
 */

 #include <stdio.h>

 #include "bsp_config.h"
 #include "bsp_video.h"
 #include "bsp_uvc.h"
 #include "bsp_sd_card.h"
 
 static const char *TAG = "APP_MAIN";
 

 // 全局变量声明
device_ctx_t *device_ctx;

// 初始化 uvc_t 结构体
void bsp_struct_alloc(void) {
    device_ctx = calloc(1, sizeof(device_ctx_t));
    if (device_ctx == NULL) {
        // 处理内存分配失败的情况
        ESP_LOGE("BSP_CONFIG", "Failed to allocate memory for uvc_t");
    }
}

 void app_main(void) {
 
     ESP_LOGI(TAG, "Initializing ----------------------------------------- ");
 
 
     bsp_struct_alloc();
     bsp_video_init(device_ctx);
 
     bsp_uvc_init(device_ctx);
 
     bsp_init_sd_card(device_ctx);
     bsp_sd_card_test(device_ctx);
     bsp_deinit_sd_card(device_ctx);

     

     ESP_LOGI(TAG, "finalizing ----------------------------------------- ");
 
 
 }
 