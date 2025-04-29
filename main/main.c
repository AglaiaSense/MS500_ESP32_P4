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
 
 static const char *TAG = "Main";
 
 extern device_ctx_t *device_ctx; // 全局变量声明
 
 void app_main(void) {
 
     ESP_LOGI(TAG, "main .......................");
 
 
     bsp_struct_alloc();
     bsp_video_init();
 
     bsp_uvc_init();
 
    //  bsp_init_sd_card(device_ctx);
    //  bsp_sd_card_test(device_ctx);
    //  bsp_deinit_sd_card(device_ctx);

     ESP_LOGI(TAG, "main end .......................");
 
 
 }
 