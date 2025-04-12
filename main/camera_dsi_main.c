/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sdkconfig.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_panel_ops.h"
#include "esp_ldo_regulator.h"
#include "esp_cache.h"
#include "driver/i2c_master.h"
#include "driver/isp.h"
#include "esp_cam_ctlr_csi.h"
#include "esp_cam_ctlr.h"
#include "example_dsi_init.h"
#include "example_dsi_init_config.h"
#include "example_sensor_init.h"
#include "example_config.h"

#include "usb_device_uvc.h"
#include "usb_cam.h"
#include "esp_spiffs.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "cam_dsi";

static bool s_camera_get_new_vb(esp_cam_ctlr_handle_t handle, esp_cam_ctlr_trans_t *trans, void *user_data);
static bool s_camera_get_finished_trans(esp_cam_ctlr_handle_t handle, esp_cam_ctlr_trans_t *trans, void *user_data);

extern  SemaphoreHandle_t frame_mutex;
// 存储接收到的帧数据
static esp_cam_ctlr_trans_t received_frame;

// 定义周期性任务
void periodic_task(void *pvParameters)
{
    int count = 0;
    while (1) {
        printf("weak task running...: %d\r\n", count++);
        vTaskDelay(pdMS_TO_TICKS(1000)); // 延时1秒
    }
}

void usb_uvc_device_init(void)
{
    printf(" _   _ _   _ _____     _____ _____ _____ _____ \n");
    printf("| | | | | | /  __ \\   |_   _|  ___/  ___|_   _|\n");
    printf("| | | | | | | /  \\/_____| | | |__ \\ `--.  | |  \n");
    printf("| | | | | | | |  |______| | |  __| `--. \\ | |\n");
    printf("| |_| \\ \\_/ / \\__/\\     | | | |___/\\__/ / | |\n");
    printf(" \\___/ \\___/ \\____/     \\_/ \\____/\\____/  \\_/\n");
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

void camerta_init(void)
{
    printf("  ____   ___    ___  _____  _____  _____  \n");
    printf(" / __ \\ / _ \\  |_ _|| ____||_   _||_   \n");
    printf("| |  | | /_\\ |  | | |  _|    | |    | |   \n");
    printf("| |  | |  |  | | | |___   | |    | |   \n");
    printf("| |__| | | | |  | | | ____|  | |    | |   \n");
    printf(" \\____/|_| |_| |___||_____|  |_|    |_|   \n");

    esp_err_t ret = ESP_FAIL;
    esp_lcd_dsi_bus_handle_t mipi_dsi_bus = NULL;
    esp_lcd_panel_io_handle_t mipi_dbi_io = NULL;
    esp_lcd_panel_handle_t mipi_dpi_panel = NULL;
    void *frame_buffer = NULL;
    size_t frame_buffer_size = 0;

    //mipi ldo
    esp_ldo_channel_handle_t ldo_mipi_phy = NULL;
    esp_ldo_channel_config_t ldo_mipi_phy_config = {
        .chan_id = CONFIG_EXAMPLE_USED_LDO_CHAN_ID,
        .voltage_mv = CONFIG_EXAMPLE_USED_LDO_VOLTAGE_MV,
    };
    ESP_ERROR_CHECK(esp_ldo_acquire_channel(&ldo_mipi_phy_config, &ldo_mipi_phy));

    /**
     * @background
     * Sensor use RAW8
     * ISP convert to RGB565
     */
    //---------------DSI Init------------------//
    example_dsi_resource_alloc(&mipi_dsi_bus, &mipi_dbi_io, &mipi_dpi_panel, &frame_buffer);

    //---------------Necessary variable config------------------//
    frame_buffer_size = CONFIG_EXAMPLE_MIPI_CSI_DISP_HRES * CONFIG_EXAMPLE_MIPI_CSI_DISP_VRES  * 2;

    ESP_LOGD(TAG, "CONFIG_EXAMPLE_MIPI_CSI_DISP_HRES: %d, CONFIG_EXAMPLE_MIPI_DSI_DISP_VRES: %d, bits per pixel: %d", CONFIG_EXAMPLE_MIPI_CSI_DISP_HRES, CONFIG_EXAMPLE_MIPI_DSI_DISP_VRES, 8);
    ESP_LOGD(TAG, "frame_buffer_size: %zu", frame_buffer_size);
    ESP_LOGD(TAG, "frame_buffer: %p", frame_buffer);

    esp_cam_ctlr_trans_t new_trans = {
        .buffer = frame_buffer,
        .buflen = frame_buffer_size,
    };

    //--------Camera Sensor and SCCB Init-----------//
    i2c_master_bus_handle_t i2c_bus_handle = NULL;
    example_sensor_init(I2C_NUM_0, &i2c_bus_handle);

    //---------------CSI Init------------------//
    esp_cam_ctlr_csi_config_t csi_config = {
        .ctlr_id = 0,
        .h_res = CONFIG_EXAMPLE_MIPI_CSI_DISP_HRES,
        .v_res = CONFIG_EXAMPLE_MIPI_CSI_DISP_VRES,
        .lane_bit_rate_mbps = EXAMPLE_MIPI_CSI_LANE_BITRATE_MBPS,
        .input_data_color_type = CAM_CTLR_COLOR_RAW8,
        .output_data_color_type =  CAM_CTLR_COLOR_YUV420,
        .data_lane_num = 2,
        .byte_swap_en = false,
        .queue_items = 1,
    };
    esp_cam_ctlr_handle_t cam_handle = NULL;
    ret = esp_cam_new_csi_ctlr(&csi_config, &cam_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "csi init fail[%d]", ret);
        return;
    }

    esp_cam_ctlr_evt_cbs_t cbs = {
        .on_get_new_trans = s_camera_get_new_vb,
        .on_trans_finished = s_camera_get_finished_trans,
    };
    if (esp_cam_ctlr_register_event_callbacks(cam_handle, &cbs, &new_trans) != ESP_OK) {
        ESP_LOGE(TAG, "ops register fail");
        return;
    }

    ESP_ERROR_CHECK(esp_cam_ctlr_enable(cam_handle));

    //---------------ISP Init------------------//
    isp_proc_handle_t isp_proc = NULL;
    esp_isp_processor_cfg_t isp_config = {
        .clk_hz = 80 * 1000 * 1000,
        .input_data_source = ISP_INPUT_DATA_SOURCE_CSI,
        .input_data_color_type = ISP_COLOR_RAW8,
        .output_data_color_type = ISP_COLOR_YUV420,
        .has_line_start_packet = false,
        .has_line_end_packet = false,
        .h_res = CONFIG_EXAMPLE_MIPI_CSI_DISP_HRES,
        .v_res = CONFIG_EXAMPLE_MIPI_CSI_DISP_VRES,
    };
    ESP_ERROR_CHECK(esp_isp_new_processor(&isp_config, &isp_proc));
    ESP_ERROR_CHECK(esp_isp_enable(isp_proc));

    //---------------DPI Reset------------------//
    // example_dpi_panel_reset(mipi_dpi_panel);

    //init to all white
    // memset(, 0xFF, frame_buffer_size);
    // esp_cache_msync(frame_buffer(void *)frame_buffer, frame_buffer_size, ESP_CACHE_MSYNC_FLAG_DIR_C2M);

    if (esp_cam_ctlr_start(cam_handle) != ESP_OK) {
        ESP_LOGE(TAG, "Driver start fail");
        return;
    }

    // example_dpi_panel_init(mipi_dpi_panel);

    fflush(stdout);
    while (1) {
        ESP_ERROR_CHECK(esp_cam_ctlr_receive(cam_handle, &new_trans, ESP_CAM_CTLR_MAX_DELAY));
    }
}

void app_main(void)
{
    printf("---------------------------------------------\n");
    // 创建周期性任务
    xTaskCreate(periodic_task, "periodic_task", 2048, NULL, 5, NULL);

    fflush(stdout);

    usb_uvc_device_init();
    camerta_init();
}

static bool s_camera_get_new_vb(esp_cam_ctlr_handle_t handle, esp_cam_ctlr_trans_t *trans, void *user_data)
{
    esp_cam_ctlr_trans_t new_trans = *(esp_cam_ctlr_trans_t *)user_data;
    trans->buffer = new_trans.buffer;
    trans->buflen = new_trans.buflen;

    return false;
}

static bool s_camera_get_finished_trans(esp_cam_ctlr_handle_t handle, esp_cam_ctlr_trans_t *trans, void *user_data)
{
    if (xSemaphoreTake(frame_mutex, portMAX_DELAY) == pdTRUE) {
        // 存储接收到的帧数据
        received_frame.buffer = trans->buffer;
        received_frame.buflen = trans->buflen;
        xSemaphoreGive(frame_mutex);
    }
    return false;
}