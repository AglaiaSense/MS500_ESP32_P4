
#include "bsp_video.h"
#include "bsp_config.h"



static const char *TAG = "BSP_VIDEO";
extern device_ctx_t *device_ctx;


static const esp_video_init_csi_config_t csi_config[] = {
    {
        .sccb_config = {
            .init_sccb = true, // 初始化SCCB
            .i2c_config = {
                .port      = CONFIG_EXAMPLE_MIPI_CSI_SCCB_I2C_PORT, // I2C端口
                .scl_pin   = CONFIG_EXAMPLE_MIPI_CSI_SCCB_I2C_SCL_PIN, // I2C SCL引脚
                .sda_pin   = CONFIG_EXAMPLE_MIPI_CSI_SCCB_I2C_SDA_PIN, // I2C SDA引脚
            },
            .freq = CONFIG_EXAMPLE_MIPI_CSI_SCCB_I2C_FREQ, // I2C频率
        },
        .reset_pin = CONFIG_EXAMPLE_MIPI_CSI_CAM_SENSOR_RESET_PIN, // 摄像头传感器复位引脚
        .pwdn_pin  = CONFIG_EXAMPLE_MIPI_CSI_CAM_SENSOR_PWDN_PIN, // 摄像头传感器电源引脚
    },
};

static const esp_video_init_config_t cam_config = {
    .csi      = csi_config, // CSI配置
};
/**
 * @brief 打印视频设备信息
 *
 * 该函数用于打印视频设备的详细信息，包括版本、驱动、卡片、总线信息以及设备的各种能力。
 *
 * @param capability 指向v4l2_capability结构体的指针，包含设备的能力信息。
 */
static void print_video_device_info(const struct v4l2_capability *capability)
{
    // 打印设备版本信息
    ESP_LOGI(TAG, "version: %d.%d.%d", (uint16_t)(capability->version >> 16),
             (uint8_t)(capability->version >> 8),
             (uint8_t)capability->version);
    
    // 打印设备驱动信息
    ESP_LOGI(TAG, "driver:  %s", capability->driver);
    
    // 打印设备卡片信息
    ESP_LOGI(TAG, "card:    %s", capability->card);
    
    // 打印设备总线信息
    ESP_LOGI(TAG, "bus:     %s", capability->bus_info);
    
    // 打印设备能力信息
    ESP_LOGI(TAG, "capabilities:");
    if (capability->capabilities & V4L2_CAP_VIDEO_CAPTURE) {
        // 打印设备是否支持视频捕获
        ESP_LOGI(TAG, "\tVIDEO_CAPTURE");
    }
    if (capability->capabilities & V4L2_CAP_READWRITE) {
        // 打印设备是否支持读写操作
        ESP_LOGI(TAG, "\tREADWRITE");
    }
    if (capability->capabilities & V4L2_CAP_ASYNCIO) {
        // 打印设备是否支持异步I/O操作
        ESP_LOGI(TAG, "\tASYNCIO");
    }
    if (capability->capabilities & V4L2_CAP_STREAMING) {
        // 打印设备是否支持流媒体操作
        ESP_LOGI(TAG, "\tSTREAMING");
    }
    if (capability->capabilities & V4L2_CAP_META_OUTPUT) {
        // 打印设备是否支持元数据输出
        ESP_LOGI(TAG, "\tMETA_OUTPUT");
    }
    if (capability->capabilities & V4L2_CAP_DEVICE_CAPS) {
        ESP_LOGI(TAG, "device capabilities:");
        if (capability->device_caps & V4L2_CAP_VIDEO_CAPTURE) {
            // 打印设备是否支持视频捕获
            ESP_LOGI(TAG, "\tVIDEO_CAPTURE");
        }
        if (capability->device_caps & V4L2_CAP_READWRITE) {
            // 打印设备是否支持读写操作
            ESP_LOGI(TAG, "\tREADWRITE");
        }
        if (capability->device_caps & V4L2_CAP_ASYNCIO) {
            // 打印设备是否支持异步I/O操作
            ESP_LOGI(TAG, "\tASYNCIO");
        }
        if (capability->device_caps & V4L2_CAP_STREAMING) {
            // 打印设备是否支持流媒体操作
            ESP_LOGI(TAG, "\tSTREAMING");
        }
        if (capability->device_caps & V4L2_CAP_META_OUTPUT) {
            // 打印设备是否支持元数据输出
            ESP_LOGI(TAG, "\tMETA_OUTPUT");
        }
    }
}

/**
 * @brief 初始化视频捕获功能
 *
 * @param uvc 指向uvc_t结构体的指针
 * @return esp_err_t 返回错误代码
 */
static esp_err_t init_capture_video(device_ctx_t *uvc)
{
    int fd;
    struct v4l2_capability capability;

    // 打开摄像头设备
    fd = open(CAM_DEV_PATH, O_RDONLY);
    assert(fd >= 0);

    // 查询摄像头设备能力
    ESP_ERROR_CHECK(ioctl(fd, VIDIOC_QUERYCAP, &capability));
    print_video_device_info(&capability);

    // 保存文件描述符到uvc结构体
    uvc->cap_fd = fd;

    return 0;
}
/**
 * @brief 初始化视频编码功能
 *
 * @param uvc 指向uvc_t结构体的指针
 * @return esp_err_t 返回错误代码
 */
static esp_err_t init_codec_video(device_ctx_t *uvc)
{
    int fd;
    const char *devpath = ENCODE_DEV_PATH;
    struct v4l2_capability capability;
    struct v4l2_ext_controls controls;
    struct v4l2_ext_control control[1];

    // 打开编码设备
    fd = open(devpath, O_RDONLY);
    assert(fd >= 0);

    // 查询编码设备能力
    ESP_ERROR_CHECK(ioctl(fd, VIDIOC_QUERYCAP, &capability));
    print_video_device_info(&capability);

#if CONFIG_FORMAT_MJPEG_CAM1
    // 设置JPEG压缩质量
    controls.ctrl_class = V4L2_CID_JPEG_CLASS;
    controls.count      = 1;
    controls.controls   = control;
    control[0].id       = V4L2_CID_JPEG_COMPRESSION_QUALITY;
    control[0].value    = CONFIG_EXAMPLE_JPEG_COMPRESSION_QUALITY;
    if (ioctl(fd, VIDIOC_S_EXT_CTRLS, &controls) != 0) {
        ESP_LOGW(TAG, "failed to set JPEG compression quality");
    }
#elif CONFIG_FORMAT_H264_CAM1
    // 设置H.264编码参数
    controls.ctrl_class = V4L2_CID_CODEC_CLASS;
    controls.count      = 1;
    controls.controls   = control;
    control[0].id       = V4L2_CID_MPEG_VIDEO_H264_I_PERIOD;
    control[0].value    = CONFIG_EXAMPLE_H264_I_PERIOD;
    if (ioctl(fd, VIDIOC_S_EXT_CTRLS, &controls) != 0) {
        ESP_LOGW(TAG, "failed to set H.264 intra frame period");
    }
    // 设置H.264编码比特率
    controls.ctrl_class = V4L2_CID_CODEC_CLASS;
    controls.count      = 1;
    controls.controls   = control;
    control[0].id       = V4L2_CID_MPEG_VIDEO_BITRATE;
    control[0].value    = CONFIG_EXAMPLE_H264_BITRATE;
    if (ioctl(fd, VIDIOC_S_EXT_CTRLS, &controls) != 0) {
        ESP_LOGW(TAG, "failed to set H.264 bitrate");
    }

    // 设置H.264编码最小质量
    controls.ctrl_class = V4L2_CID_CODEC_CLASS;
    controls.count      = 1;
    controls.controls   = control;
    control[0].id       = V4L2_CID_MPEG_VIDEO_H264_MIN_QP;
    control[0].value    = CONFIG_EXAMPLE_H264_MIN_QP;
    if (ioctl(fd, VIDIOC_S_EXT_CTRLS, &controls) != 0) {
        ESP_LOGW(TAG, "failed to set H.264 minimum quality");
    }

    // 设置H.264编码最大质量
    controls.ctrl_class = V4L2_CID_CODEC_CLASS;
    controls.count      = 1;
    controls.controls   = control;
    control[0].id       = V4L2_CID_MPEG_VIDEO_H264_MAX_QP;
    control[0].value    = CONFIG_EXAMPLE_H264_MAX_QP;
    if (ioctl(fd, VIDIOC_S_EXT_CTRLS, &controls) != 0) {
        ESP_LOGW(TAG, "failed to set H.264 maximum quality");
    }
#endif

    // 保存编码格式和文件描述符到uvc结构体
    uvc->format = UVC_OUTPUT_FORMAT;
    uvc->m2m_fd = fd;

    return 0;
}


void bsp_video_init(void) {
    ESP_LOGI(TAG, "Initializing .......................");
    
    // 初始化摄像头配置
    ESP_ERROR_CHECK(esp_video_init(&cam_config));
    
    // 初始化视频捕获功能
    ESP_ERROR_CHECK(init_capture_video(device_ctx));
    
    // 初始化视频编码功能
    ESP_ERROR_CHECK(init_codec_video(device_ctx));


}
 