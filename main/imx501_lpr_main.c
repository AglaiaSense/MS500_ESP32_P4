/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

// #include <inttypes.h>
// #include <stdio.h>
// #include <string.h>

// #include "cJSON.h"
// #include "esp_chip_info.h"
// #include "esp_flash.h"
// #include "esp_log.h"
// #include "esp_system.h"
// #include "esp_spiffs.h"

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

void imx501_lpr_main(void) {
    // ------------------------ 初始化硬件 ------------------------
    ai_gpio_init();

    bsp_spiff_init();

    spi_master_dev_init();

    ai_i2c_master_init();

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

/* 主函数 */
// void app_main(void) {
//     imx501_lpr_main();
// }
