/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "bsp_uvc_jpg.h"

#include "esp_log.h"
#include "esp_timer.h"

#include <inttypes.h>

#define WIDTH CONFIG_UVC_CAM2_FRAMESIZE_WIDTH
#define HEIGHT CONFIG_UVC_CAM2_FRAMESIZE_HEIGT

#define UVC_MAX_FRAMESIZE_SIZE (350 * 1024)

static const char *TAG = "usb_cam2";
static uvc_fb_t s_fb;

static void camera_stop_cb(void *cb_ctx) {
    (void)cb_ctx;

    ESP_LOGI(TAG, "Camera:%" PRIu32 " Stop", (uint32_t)cb_ctx);
}

static esp_err_t camera_start_cb(uvc_format_t format, int width, int height, int rate, void *cb_ctx) {
    (void)cb_ctx;
    ESP_LOGI(TAG, "Camera:%" PRIu32 " Start", (uint32_t)cb_ctx);
    ESP_LOGI(TAG, "Format: %d, width: %d, height: %d, rate: %d", format, width, height, rate);

    return ESP_OK;
}

static uvc_fb_t *camera_fb_get_cb(void *cb_ctx) {

    (void)cb_ctx;
    uint64_t us = (uint64_t)esp_timer_get_time();

    // 打开SPIFFS中的图片文件
    FILE *file = fopen("/spiffs/150.jpg", "rb");
    if (!file) {
        ESP_LOGE(TAG, "Failed to open image file");
        return NULL;
    }

    // 获取文件大小
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // printf("%s(%d)\n", __func__, __LINE__);

    // 分配内存并读取文件内容
    uint8_t *jpg_data = malloc(file_size);
    if (!jpg_data) {
        ESP_LOGE(TAG, "Failed to allocate memory for image");
        fclose(file);
        return NULL;
    }

    fread(jpg_data, 1, file_size, file);
    fclose(file);


    printf("%s(%d)\n", __func__, __LINE__);

    s_fb.buf = jpg_data;
    s_fb.len = file_size;
    s_fb.width = WIDTH;
    s_fb.height = HEIGHT;
    s_fb.format = UVC_FORMAT_JPEG;
    s_fb.timestamp.tv_sec = us / 1000000UL;
    s_fb.timestamp.tv_usec = us % 1000000UL;

    return &s_fb;
}

static void camera_fb_return_cb(uvc_fb_t *fb, void *cb_ctx) {
    (void)cb_ctx;
    assert(fb == &s_fb);
    if (fb->buf) {
        free(fb->buf);
        fb->buf = NULL;
    }
}

esp_err_t usb_cam2_init(void) {
    int index = 1;

    uint8_t *uvc_buffer = (uint8_t *)malloc(UVC_MAX_FRAMESIZE_SIZE);
    if (uvc_buffer == NULL) {
        ESP_LOGE(TAG, "malloc frame buffer fail");
        return ESP_FAIL;
    }

    uvc_device_config_t config = {
        .uvc_buffer = uvc_buffer,
        .uvc_buffer_size = UVC_MAX_FRAMESIZE_SIZE,
        .start_cb = camera_start_cb,
        .fb_get_cb = camera_fb_get_cb,
        .fb_return_cb = camera_fb_return_cb,
        .stop_cb = camera_stop_cb,
        .cb_ctx = (void *)2,
    };

    ESP_LOGI(TAG, "Format List");
    ESP_LOGI(TAG, "\tFormat(1) = %s", "MJPEG");
    ESP_LOGI(TAG, "Frame List");
    ESP_LOGI(TAG, "\tFrame(1) = %d * %d @%dfps", UVC_FRAMES_INFO[index][0].width, UVC_FRAMES_INFO[index][0].height, UVC_FRAMES_INFO[index][0].rate);
#if CONFIG_UVC_CAM2_MULTI_FRAMESIZE
    ESP_LOGI(TAG, "\tFrame(2) = %d * %d @%dfps", UVC_FRAMES_INFO[index][1].width, UVC_FRAMES_INFO[index][1].height, UVC_FRAMES_INFO[index][1].rate);
    ESP_LOGI(TAG, "\tFrame(3) = %d * %d @%dfps", UVC_FRAMES_INFO[index][2].width, UVC_FRAMES_INFO[index][2].height, UVC_FRAMES_INFO[index][2].rate);
    ESP_LOGI(TAG, "\tFrame(3) = %d * %d @%dfps", UVC_FRAMES_INFO[index][3].width, UVC_FRAMES_INFO[index][3].height, UVC_FRAMES_INFO[index][3].rate);
#endif

    return uvc_device_config(index, &config);
}
