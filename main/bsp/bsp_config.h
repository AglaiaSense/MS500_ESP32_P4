// FILE: e:/03-MS500-P4/01.code/MS500_ESP32_P4/main/bsp/bsp_config.h
// FILE: e:/03-MS500-P4/01.code/MS500_ESP32_P4/main/bsp/bsp_config.h

#ifndef BSP_CONFIG_H
#define BSP_CONFIG_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "esp_log.h"
#include "esp_err.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/time.h>

#include "linux/videodev2.h"
#include "esp_video_init.h"
#include "esp_video_device.h"

#include "uvc_frame_config.h"
#include "usb_device_uvc.h"


#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
// #include "sd_test_io.h"
#if SOC_SDMMC_IO_POWER_EXTERNAL
#include "sd_pwr_ctrl_by_on_chip_ldo.h"
#endif


// 常用宏定义

#define BUFFER_COUNT        2
#define CAM_DEV_PATH        ESP_VIDEO_MIPI_CSI_DEVICE_NAME

#define SKIP_STARTUP_FRAME_COUNT   2

#define CONFIG_EXAMPLE_H264_I_PERIOD 120
#define CONFIG_EXAMPLE_H264_BITRATE 1000000
#define CONFIG_EXAMPLE_H264_MIN_QP 25
#define CONFIG_EXAMPLE_H264_MAX_QP 26

#define CONFIG_EXAMPLE_JPEG_COMPRESSION_QUALITY 80

// SD卡相关
#define MOUNT_POINT "/sdcard"


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


 
/**
 * @brief SD card encoder image data format
 */
typedef enum {
    SD_IMG_FORMAT_JPEG,            /*!< JPEG format */
    SD_IMG_FORMAT_H264,            /*!< H264 format */
} sd_image_format_t;

/* SD Card framebuffer type */
typedef struct sd_card_fb {
    uint8_t *buf;
    size_t buf_bytesused;
    sd_image_format_t fmt;
    size_t width;               /*!< Width of the image frame in pixels */
    size_t height;              /*!< Height of the image frame in pixels */
    struct timeval timestamp;   /*!< Timestamp since boot of the frame */
} sd_card_fb_t;

typedef struct device_ctx {
    int cap_fd;
    uint32_t format;
    uint8_t *cap_buffer[BUFFER_COUNT];

    int m2m_fd;
    uint8_t *m2m_cap_buffer;

    uvc_fb_t fb;

    sd_card_fb_t sd_fb;
    sdmmc_card_t *card;
#if CONFIG_EXAMPLE_SD_PWR_CTRL_LDO_INTERNAL_IO
    sd_pwr_ctrl_handle_t pwr_ctrl_handle;  /*!< Power control handle */
#endif

} device_ctx_t;

 
// 函数声明



#endif // BSP_CONFIG_H
