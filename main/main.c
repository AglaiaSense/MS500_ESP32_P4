/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: ESPRESSIF MIT
 */

#include <stdio.h>

#include "bsp_config.h"
#include "bsp_video.h"
#include "bsp_uvc.h"

static const char *TAG = "Main";


void app_main(void) {

    ESP_LOGI(TAG, "Initializing .......................");


    bsp_struct_alloc();
    bsp_video_init();

    bsp_uvc_init();
}
