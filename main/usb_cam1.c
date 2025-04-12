/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 #include <string.h>
 #include <inttypes.h>
 #include "freertos/FreeRTOS.h"
 #include "freertos/semphr.h"
 #include "esp_log.h"
 #include "avi_player.h"
 #include "usb_cam.h"
 #include "camera_dsi_main.c"  // 包含头文件以访问 received_frame
 #include "freertos/semphr.h"

//  #define WIDTH  CONFIG_UVC_CAM1_FRAMESIZE_WIDTH
//  #define HEIGHT CONFIG_UVC_CAM1_FRAMESIZE_HEIGT
 
SemaphoreHandle_t frame_mutex;

 static  const char *TAG_UVC = "usb_cam1";
 static uvc_fb_t s_fb;
 static bool running = false;
 
 static void camera_stop_cb(void *cb_ctx)
 {
     (void)cb_ctx;
     if (running) {
        //  avi_player_play_stop();
         running = false;
     }
     ESP_LOGI(TAG_UVC, "Camera:%"PRIu32" Stop", (uint32_t)cb_ctx);
 }
 
 static esp_err_t camera_start_cb(uvc_format_t format, int width, int height, int rate, void *cb_ctx)
 {
     (void)cb_ctx;
     ESP_LOGI(TAG_UVC, "Camera:%"PRIu32" Start", (uint32_t)cb_ctx);
     ESP_LOGI(TAG_UVC, "Format: %d, width: %d, height: %d, rate: %d", format, width, height, rate);
 
     running = true;
     // 不再播放视频文件
     // avi_player_play_from_file("/spiffs/p4_introduce.avi");
 
     return ESP_OK;
 }
 
static uvc_fb_t* camera_fb_get_cb(void *cb_ctx)
{
    (void)cb_ctx;
    if (xSemaphoreTake(frame_mutex, portMAX_DELAY) == pdTRUE) {
        if (received_frame.buffer != NULL && received_frame.buflen > 0) {
            if (received_frame.buflen <= UVC_MAX_FRAMESIZE_SIZE) {
                memcpy(s_fb.buf, received_frame.buffer, received_frame.buflen);
                s_fb.len = received_frame.buflen;
                s_fb.width = CONFIG_EXAMPLE_MIPI_CSI_DISP_HRES;
                s_fb.height = CONFIG_EXAMPLE_MIPI_CSI_DISP_VRES;
// #if CONFIG_FORMAT_MJPEG_CAM1
//                 s_fb.format = UVC_FORMAT_JPEG;
// #else
                s_fb.format = UVC_FORMAT_H264;
// #endif

                uint64_t us = (uint64_t)esp_timer_get_time();
                s_fb.timestamp.tv_sec = us / 1000000UL;
                s_fb.timestamp.tv_usec = us % 1000000UL;

                xSemaphoreGive(frame_mutex);
                return &s_fb;
            } else {
                ESP_LOGE(TAG_UVC, "Frame size %d is larger than max frame size %d", received_frame.buflen, UVC_MAX_FRAMESIZE_SIZE);
            }
        }
        xSemaphoreGive(frame_mutex);
    }
    return NULL;
}
 
 static void camera_fb_return_cb(uvc_fb_t *fb, void *cb_ctx)
 {
     (void)cb_ctx;
     assert(fb == &s_fb);
 }
 
 static void avi_end_cb(void *arg)
 {
     if (running) {
         // 不再播放视频文件
         // avi_player_play_from_file("/spiffs/p4_introduce.avi");
     }
 }
 
 esp_err_t usb_cam1_init(void)
 {
    frame_mutex = xSemaphoreCreateMutex();
    if (frame_mutex == NULL) {
        ESP_LOGE(TAG_UVC, "Failed to create mutex");
        return ESP_FAIL;
    }

     avi_player_config_t avi_cfg = {
         .avi_play_end_cb = avi_end_cb,
         .buffer_size = UVC_MAX_FRAMESIZE_SIZE,
     };
 
    //  avi_player_init(avi_cfg);
 
     s_fb.buf = (uint8_t *)malloc(UVC_MAX_FRAMESIZE_SIZE);
     if (s_fb.buf == NULL) {
         ESP_LOGE(TAG_UVC, "malloc frame buffer fail");
         return ESP_FAIL;
     }
 
     uint32_t index = 1;
     uint8_t *uvc_buffer = (uint8_t *)malloc(UVC_MAX_FRAMESIZE_SIZE);
     if (uvc_buffer == NULL) {
         ESP_LOGE(TAG_UVC, "malloc frame buffer fail");
         return ESP_FAIL;
     }
 
     uvc_device_config_t config = {
         .uvc_buffer = uvc_buffer,
         .uvc_buffer_size = UVC_MAX_FRAMESIZE_SIZE,
         .start_cb = camera_start_cb,
         .fb_get_cb = camera_fb_get_cb,
         .fb_return_cb = camera_fb_return_cb,
         .stop_cb = camera_stop_cb,
         .cb_ctx = (void *)index,
     };
 
     ESP_LOGI(TAG_UVC, "Format List");
 #if CONFIG_FORMAT_MJPEG_CAM1
     ESP_LOGI(TAG_UVC, "\tFormat(1) = %s", "MJPEG");
 #else
     ESP_LOGI(TAG_UVC, "\tFormat(1) = %s", "H264");
 #endif
     ESP_LOGI(TAG_UVC, "Frame List");
     ESP_LOGI(TAG_UVC, "\tFrame(1) = %d * %d @%dfps", UVC_FRAMES_INFO[index - 1][0].width, UVC_FRAMES_INFO[index - 1][0].height, UVC_FRAMES_INFO[index - 1][0].rate);
 #if CONFIG_UVC_CAM1_MULTI_FRAMESIZE
     ESP_LOGI(TAG_UVC, "\tFrame(2) = %d * %d @%dfps", UVC_FRAMES_INFO[index - 1][1].width, UVC_FRAMES_INFO[index - 1][1].height, UVC_FRAMES_INFO[index - 1][1].rate);
     ESP_LOGI(TAG_UVC, "\tFrame(3) = %d * %d @%dfps", UVC_FRAMES_INFO[index - 1][2].width, UVC_FRAMES_INFO[index - 1][2].height, UVC_FRAMES_INFO[index - 1][2].rate);
     ESP_LOGI(TAG_UVC, "\tFrame(3) = %d * %d @%dfps", UVC_FRAMES_INFO[index - 1][3].width, UVC_FRAMES_INFO[index - 1][3].height, UVC_FRAMES_INFO[index - 1][3].rate);
 #endif
 
     return uvc_device_config(index - 1, &config);
 }