// FILE: e:/03-MS500-P4/01.code/MS500_ESP32_P4/main/bsp/bsp_config.h
// FILE: e:/03-MS500-P4/01.code/MS500_ESP32_P4/main/bsp/bsp_config.h

#ifndef BSP_CONFIG_H
#define BSP_CONFIG_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "esp_log.h"
#include "esp_err.h"

#include <sys/mman.h>
#include <sys/param.h>
#include <sys/errno.h>


#include <fcntl.h>
#include <sys/ioctl.h>

#include "linux/videodev2.h"
#include "esp_video_init.h"
#include "esp_video_device.h"

#include "uvc_frame_config.h"
#include "usb_device_uvc.h"


// 常用宏定义

#define BUFFER_COUNT        2
#define CAM_DEV_PATH        ESP_VIDEO_MIPI_CSI_DEVICE_NAME


#if CONFIG_FORMAT_MJPEG_CAM1
#define ENCODE_DEV_PATH     ESP_VIDEO_JPEG_DEVICE_NAME
#define UVC_OUTPUT_FORMAT   V4L2_PIX_FMT_JPEG
#elif CONFIG_FORMAT_H264_CAM1

#if CONFIG_EXAMPLE_H264_MAX_QP <= CONFIG_EXAMPLE_H264_MIN_QP
#error "CONFIG_EXAMPLE_H264_MAX_QP should larger than CONFIG_EXAMPLE_H264_MIN_QP"
#endif

#define ENCODE_DEV_PATH     ESP_VIDEO_H264_DEVICE_NAME
#define UVC_OUTPUT_FORMAT   V4L2_PIX_FMT_H264
#endif



// 通用结构体定义
typedef struct device_ctx {
    int cap_fd;
    uint32_t format;
    uint8_t *cap_buffer[BUFFER_COUNT];

    int m2m_fd;
    uint8_t *m2m_cap_buffer;

    uvc_fb_t fb;
} device_ctx_t;

 
// 函数声明

void bsp_struct_alloc(void) ;



#endif // BSP_CONFIG_H
