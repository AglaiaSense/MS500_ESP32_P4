#include "bsp_uvc_cam.h"
#include "bsp_config.h"

#include "esp_timer.h"

static const char *TAG = "BSP_UVC_CAM";

static size_t s_cap_buf_len[BUFFER_COUNT];
static size_t s_m2m_cap_buf_len;

extern esp_err_t store_jpg_to_sd_card(device_ctx_t *sd);

static esp_err_t video_start_cb(uvc_format_t uvc_format, int width, int height, int rate, void *cb_ctx) {
    int type;
    struct v4l2_buffer buf;
    struct v4l2_format format;
    struct v4l2_requestbuffers req;
    struct v4l2_format init_format;

    device_ctx_t *uvc = (device_ctx_t *)cb_ctx; // 获取UVC设备上下文
    uint32_t capture_fmt = 0;                   // 初始化捕获格式

    ESP_LOGI(TAG, "UVC start"); // 记录UVC启动日志

    // 处理JPEG格式
    if (uvc->format == V4L2_PIX_FMT_JPEG) {
        int fmt_index = 0;
        // 定义支持的JPEG输入格式列表
        const uint32_t jpeg_input_formats[] = {
            V4L2_PIX_FMT_RGB565,
            V4L2_PIX_FMT_YUV422P,
            V4L2_PIX_FMT_RGB24,
            V4L2_PIX_FMT_GREY};
        int jpeg_input_formats_num = sizeof(jpeg_input_formats) / sizeof(jpeg_input_formats[0]);

        // 枚举并匹配支持的格式
        while (!capture_fmt) {
            struct v4l2_fmtdesc fmtdesc = {
                .index = fmt_index++,
                .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
            };

            // 枚举视频格式
            if (ioctl(uvc->cap_fd, VIDIOC_ENUM_FMT, &fmtdesc) != 0) {
                break;
            }

            // 检查是否支持当前格式
            for (int i = 0; i < jpeg_input_formats_num; i++) {
                if (jpeg_input_formats[i] == fmtdesc.pixelformat) {
                    capture_fmt = jpeg_input_formats[i];
                    break;
                }
            }
        }

        // 如果没有找到支持的格式
        if (!capture_fmt) {
            ESP_LOGI(TAG, "The camera sensor output pixel format is not supported by JPEG");
            return ESP_ERR_NOT_SUPPORTED;
        }
        printf("V4L2_PIX_FMT_JPEG\n");

    } else if (uvc->format == V4L2_PIX_FMT_H264) {
        printf("V4L2_PIX_FMT_H264\n");

        /* Todo, fix input format when h264 encoder supports other formats */
        capture_fmt = V4L2_PIX_FMT_YUV420;
    }

    // 配置摄像头捕获流
    memset(&format, 0, sizeof(format));
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.width = width;
    format.fmt.pix.height = height;
    format.fmt.pix.pixelformat = capture_fmt;
    ESP_ERROR_CHECK(ioctl(uvc->cap_fd, VIDIOC_S_FMT, &format));

    // 请求视频缓冲区
    memset(&req, 0, sizeof(req));
    req.count = BUFFER_COUNT;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    ESP_ERROR_CHECK(ioctl(uvc->cap_fd, VIDIOC_REQBUFS, &req));

    // 映射内存缓冲区
    for (int i = 0; i < BUFFER_COUNT; i++) {
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        ESP_ERROR_CHECK(ioctl(uvc->cap_fd, VIDIOC_QUERYBUF, &buf));
        s_cap_buf_len[i] = buf.length; // <— 记录下长度

        uvc->cap_buffer[i] = (uint8_t *)mmap(NULL, buf.length, PROT_READ | PROT_WRITE,
                                             MAP_SHARED, uvc->cap_fd, buf.m.offset);
        assert(uvc->cap_buffer[i]);

        ESP_ERROR_CHECK(ioctl(uvc->cap_fd, VIDIOC_QBUF, &buf));
    }

    // 配置编码器输出流
    memset(&format, 0, sizeof(format));
    format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    format.fmt.pix.width = width;
    format.fmt.pix.height = height;
    format.fmt.pix.pixelformat = capture_fmt;
    ESP_ERROR_CHECK(ioctl(uvc->m2m_fd, VIDIOC_S_FMT, &format));

    // 请求编码器输出缓冲区
    memset(&req, 0, sizeof(req));
    req.count = 1;
    req.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    req.memory = V4L2_MEMORY_USERPTR;
    ESP_ERROR_CHECK(ioctl(uvc->m2m_fd, VIDIOC_REQBUFS, &req));

    // 配置编码器捕获流
    memset(&format, 0, sizeof(format));
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.width = width;
    format.fmt.pix.height = height;
    format.fmt.pix.pixelformat = uvc->format;
    ESP_ERROR_CHECK(ioctl(uvc->m2m_fd, VIDIOC_S_FMT, &format));

    // 请求编码器捕获缓冲区
    memset(&req, 0, sizeof(req));
    req.count = 1;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    ESP_ERROR_CHECK(ioctl(uvc->m2m_fd, VIDIOC_REQBUFS, &req));

    // 查询并映射编码器捕获缓冲区
    memset(&buf, 0, sizeof(buf));                               // 清空缓冲区结构
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;                     // 设置缓冲区类型为视频捕获
    buf.memory = V4L2_MEMORY_MMAP;                              // 设置内存映射方式
    buf.index = 0;                                              // 设置缓冲区索引为0
    ESP_ERROR_CHECK(ioctl(uvc->m2m_fd, VIDIOC_QUERYBUF, &buf)); // 查询缓冲区信息
    s_m2m_cap_buf_len = buf.length;                             // <— 记录下长度

    // 映射内存到用户空间
    uvc->m2m_cap_buffer = (uint8_t *)mmap(NULL, buf.length, PROT_READ | PROT_WRITE,
                                          MAP_SHARED, uvc->m2m_fd, buf.m.offset);
    assert(uvc->m2m_cap_buffer); // 确保内存映射成功

    // 将缓冲区加入编码器捕获队列
    ESP_ERROR_CHECK(ioctl(uvc->m2m_fd, VIDIOC_QBUF, &buf)); // 将查询到的缓冲区加入编码器捕获队列

    // 启动编码器捕获
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ESP_ERROR_CHECK(ioctl(uvc->m2m_fd, VIDIOC_STREAMON, &type));

    // 启动编码器输出流
    type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    ESP_ERROR_CHECK(ioctl(uvc->m2m_fd, VIDIOC_STREAMON, &type));

    // 启动摄像头捕获流
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ESP_ERROR_CHECK(ioctl(uvc->cap_fd, VIDIOC_STREAMON, &type));

    /* Skip the first few frames of the image to get a stable image. */
    for (int i = 0; i < SKIP_STARTUP_FRAME_COUNT; i++) {
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        ESP_ERROR_CHECK(ioctl(uvc->cap_fd, VIDIOC_DQBUF, &buf));
        ESP_ERROR_CHECK(ioctl(uvc->cap_fd, VIDIOC_QBUF, &buf));
    }

    /* Init sd encoder frame buffer's basic info. */
    // uvc->sd_fb.width = width;
    // uvc->sd_fb.height = height;
    // uvc->sd_fb.fmt = sd->format == V4L2_PIX_FMT_JPEG ? SD_IMG_FORMAT_JPEG : SD_IMG_FORMAT_H264;

    return ESP_OK;
}

static void video_stop_cb(void *cb_ctx) {
    int type;
    device_ctx_t *uvc = (device_ctx_t *)cb_ctx; // 获取UVC设备上下文

    ESP_LOGI(TAG, "UVC stop"); // 记录UVC停止日志

    // 停止摄像头捕获流
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(uvc->cap_fd, VIDIOC_STREAMOFF, &type);

    // 停止编码器输出流
    type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    ioctl(uvc->m2m_fd, VIDIOC_STREAMOFF, &type);

    // 停止编码器捕获流
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(uvc->m2m_fd, VIDIOC_STREAMOFF, &type);
}

static esp_err_t example_write_file(FILE *f, const uint8_t *data, size_t len) {
    size_t written;

    do {
        written = fwrite(data, 1, len, f);
        len -= written;
        data += written;
    } while (written && len);
    fflush(f);

    return ESP_OK;
}

static uvc_fb_t *video_fb_get_cb(void *cb_ctx) {
    int64_t us;
    device_ctx_t *uvc = (device_ctx_t *)cb_ctx; // 获取UVC设备上下文
    struct v4l2_format format;
    struct v4l2_buffer cap_buf;     // 摄像头捕获缓冲区
    struct v4l2_buffer m2m_out_buf; // 编码器输出缓冲区
    struct v4l2_buffer m2m_cap_buf; // 编码器捕获缓冲区

    ESP_LOGD(TAG, "UVC get"); // 记录获取帧缓冲区日志

    // 从摄像头捕获队列中获取一个缓冲区
    memset(&cap_buf, 0, sizeof(cap_buf));
    cap_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    cap_buf.memory = V4L2_MEMORY_MMAP;
    ESP_ERROR_CHECK(ioctl(uvc->cap_fd, VIDIOC_DQBUF, &cap_buf));

    // 配置编码器输出缓冲区
    memset(&m2m_out_buf, 0, sizeof(m2m_out_buf));
    m2m_out_buf.index = 0;
    m2m_out_buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    m2m_out_buf.memory = V4L2_MEMORY_USERPTR;
    m2m_out_buf.m.userptr = (unsigned long)uvc->cap_buffer[cap_buf.index]; // 使用摄像头捕获缓冲区作为输入
    m2m_out_buf.length = cap_buf.bytesused;                                // 设置数据长度
    ESP_ERROR_CHECK(ioctl(uvc->m2m_fd, VIDIOC_QBUF, &m2m_out_buf));

    // 从编码器捕获队列中获取一个缓冲区
    memset(&m2m_cap_buf, 0, sizeof(m2m_cap_buf));
    m2m_cap_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    m2m_cap_buf.memory = V4L2_MEMORY_MMAP;
    ESP_ERROR_CHECK(ioctl(uvc->m2m_fd, VIDIOC_DQBUF, &m2m_cap_buf));

    // 将摄像头捕获缓冲区重新加入队列
    ESP_ERROR_CHECK(ioctl(uvc->cap_fd, VIDIOC_QBUF, &cap_buf));
    // 将编码器输出缓冲区重新加入队列
    ESP_ERROR_CHECK(ioctl(uvc->m2m_fd, VIDIOC_DQBUF, &m2m_out_buf));

    // 获取当前视频格式
    memset(&format, 0, sizeof(format));
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ESP_ERROR_CHECK(ioctl(uvc->m2m_fd, VIDIOC_G_FMT, &format));

    // 填充帧缓冲区信息
    uvc->fb.buf = uvc->m2m_cap_buffer;                                                                    // 设置缓冲区地址
    uvc->fb.len = m2m_cap_buf.bytesused;                                                                  // 设置数据长度
    uvc->fb.width = format.fmt.pix.width;                                                                 // 设置帧宽度
    uvc->fb.height = format.fmt.pix.height;                                                               // 设置帧高度
    uvc->fb.format = format.fmt.pix.pixelformat == V4L2_PIX_FMT_JPEG ? UVC_FORMAT_JPEG : UVC_FORMAT_H264; // 设置帧格式

    // 获取当前时间戳
    us = esp_timer_get_time();
    uvc->fb.timestamp.tv_sec = us / 1000000UL;  // 设置秒数
    uvc->fb.timestamp.tv_usec = us % 1000000UL; // 设置微秒数

    store_jpg_to_sd_card(uvc); // 将JPEG数据存储到SD卡

    return &uvc->fb; // 返回帧缓冲区指针
}

static void video_fb_return_cb(uvc_fb_t *fb, void *cb_ctx) {
    struct v4l2_buffer m2m_cap_buf;             // 定义编码器捕获缓冲区结构
    device_ctx_t *uvc = (device_ctx_t *)cb_ctx; // 获取UVC设备上下文

    ESP_LOGD(TAG, "UVC return"); // 记录帧缓冲区返回日志

    // 配置编码器捕获缓冲区
    m2m_cap_buf.index = 0;                          // 设置缓冲区索引
    m2m_cap_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; // 设置缓冲区类型为视频捕获
    m2m_cap_buf.memory = V4L2_MEMORY_MMAP;          // 设置内存映射方式

    // 将缓冲区重新加入编码器捕获队列
    ESP_ERROR_CHECK(ioctl(uvc->m2m_fd, VIDIOC_QBUF, &m2m_cap_buf));
}

// 初始化 UVC 设备

void bsp_uvc_init(device_ctx_t *device_ctx) {

    ESP_LOGI(TAG, "Initializing ----------------------------------------- ");

    int index = 0;
    uvc_device_config_t config = {
        .start_cb = video_start_cb,
        .fb_get_cb = video_fb_get_cb,
        .fb_return_cb = video_fb_return_cb,
        .stop_cb = video_stop_cb,
        .cb_ctx = (void *)device_ctx,
    };

    // 计算并分配UVC缓冲区大小
    config.uvc_buffer_size = UVC_FRAMES_INFO[index][0].width * UVC_FRAMES_INFO[index][0].height;
    config.uvc_buffer = malloc(config.uvc_buffer_size);
    assert(config.uvc_buffer); // 确保内存分配成功

    // 打印格式信息
    ESP_LOGI(TAG, "Format List");
    ESP_LOGI(TAG, "\tFormat(1) = %s", device_ctx->format == V4L2_PIX_FMT_JPEG ? "MJPEG" : "H.264");

    // 打印帧信息
    ESP_LOGI(TAG, "\tFrame(1) = %d * %d @%dfps",
             UVC_FRAMES_INFO[index][0].width,
             UVC_FRAMES_INFO[index][0].height,
             UVC_FRAMES_INFO[index][0].rate);

    // 配置并初始化UVC设备
    ESP_ERROR_CHECK(uvc_device_config(index, &config));
}

void bsp_uvc_deinit(device_ctx_t *uvc) {
    int type;
    struct v4l2_requestbuffers req = {0};

    ESP_LOGI(TAG, "BSP UVC Deinit ----------------------------------------- ");

    // 1) 停掉所有流
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (uvc->cap_fd >= 0)
        ioctl(uvc->cap_fd, VIDIOC_STREAMOFF, &type);
    type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    if (uvc->m2m_fd >= 0)
        ioctl(uvc->m2m_fd, VIDIOC_STREAMOFF, &type);
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (uvc->m2m_fd >= 0)
        ioctl(uvc->m2m_fd, VIDIOC_STREAMOFF, &type);

    // 2) 释放内核端 buffer
    req.count = 0;
    req.memory = V4L2_MEMORY_MMAP;
    if (uvc->cap_fd >= 0) {
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        ioctl(uvc->cap_fd, VIDIOC_REQBUFS, &req);
    }
    if (uvc->m2m_fd >= 0) {
        req.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        ioctl(uvc->m2m_fd, VIDIOC_REQBUFS, &req);
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        ioctl(uvc->m2m_fd, VIDIOC_REQBUFS, &req);
    }

    // 3) munmap 应用层映射
    for (int i = 0; i < BUFFER_COUNT; i++) {
        if (uvc->cap_buffer[i] && s_cap_buf_len[i] > 0) {
            munmap(uvc->cap_buffer[i], s_cap_buf_len[i]);
            uvc->cap_buffer[i] = NULL;
            s_cap_buf_len[i] = 0;
        }
    }
    if (uvc->m2m_cap_buffer && s_m2m_cap_buf_len > 0) {
        munmap(uvc->m2m_cap_buffer, s_m2m_cap_buf_len);
        uvc->m2m_cap_buffer = NULL;
        s_m2m_cap_buf_len = 0;
    }

    // 4) 关闭设备
    if (uvc->cap_fd >= 0) {
        close(uvc->cap_fd);
        uvc->cap_fd = -1;
    }
    if (uvc->m2m_fd >= 0) {
        close(uvc->m2m_fd);
        uvc->m2m_fd = -1;
    }

    // 5) 释放 UVC 层 buffer（动态分配的）
    // if (uvc->uvc_buffer) {
    //     free(uvc->uvc_buffer);
    //     uvc->uvc_buffer = NULL;
    // }

    // 6) 停掉 UVC 任务/PHY
    // if (uvc_device_deinit() != ESP_OK) {
    //     ESP_LOGW(TAG, "uvc_device_deinit failed");
    // }

    // 7) 重置状态
    uvc->format = 0;

    ESP_LOGI(TAG, "BSP UVC Deinit complete");
}