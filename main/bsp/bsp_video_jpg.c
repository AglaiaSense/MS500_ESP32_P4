#include "bsp_video_jpg.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
 #include "esp_timer.h"
static const char *TAG = "bsp_video_jpg";

static esp_err_t example_video_start(device_ctx_t *sd)
{
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    struct v4l2_buffer buf;
    struct v4l2_format format;
    struct v4l2_requestbuffers req;
    struct v4l2_format init_format;
    uint32_t capture_fmt = 0;
    uint32_t width, height;

    ESP_LOGD(TAG, "Video start");

    memset(&init_format, 0, sizeof(struct v4l2_format));
    init_format.type = type;
    if (ioctl(sd->cap_fd, VIDIOC_G_FMT, &init_format) != 0) {
        ESP_LOGE(TAG, "failed to get format");
        return ESP_FAIL;
    }
    /* Use default length and width, refer to the sensor format options in menuconfig. */
    width = init_format.fmt.pix.width;
    height = init_format.fmt.pix.height;

    if (sd->format == V4L2_PIX_FMT_JPEG) {
        int fmt_index = 0;
        const uint32_t jpeg_input_formats[] = {
            V4L2_PIX_FMT_RGB565,
            V4L2_PIX_FMT_YUV422P,
            V4L2_PIX_FMT_RGB24,
            V4L2_PIX_FMT_GREY
        };
        int jpeg_input_formats_num = sizeof(jpeg_input_formats) / sizeof(jpeg_input_formats[0]);

        while (!capture_fmt) {
            struct v4l2_fmtdesc fmtdesc = {
                .index = fmt_index++,
                .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
            };

            if (ioctl(sd->cap_fd, VIDIOC_ENUM_FMT, &fmtdesc) != 0) {
                break;
            }

            for (int i = 0; i < jpeg_input_formats_num; i++) {
                if (jpeg_input_formats[i] == fmtdesc.pixelformat) {
                    capture_fmt = jpeg_input_formats[i];
                    break;
                }
            }
        }

        if (!capture_fmt) {
            ESP_LOGI(TAG, "The camera sensor output pixel format is not supported by JPEG encoder");
            return ESP_ERR_NOT_SUPPORTED;
        }
    } else if (sd->format == V4L2_PIX_FMT_H264) {
        /* Todo, fix input format when h264 encoder supports other formats */
        capture_fmt = V4L2_PIX_FMT_YUV420;
    }

    /* Configure camera interface capture stream */

    memset(&format, 0, sizeof(format));
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.width = width;
    format.fmt.pix.height = height;
    format.fmt.pix.pixelformat = capture_fmt;
    ESP_ERROR_CHECK(ioctl(sd->cap_fd, VIDIOC_S_FMT, &format));

    memset(&req, 0, sizeof(req));
    req.count  = BUFFER_COUNT;
    req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    ESP_ERROR_CHECK(ioctl(sd->cap_fd, VIDIOC_REQBUFS, &req));

    for (int i = 0; i < BUFFER_COUNT; i++) {
        memset(&buf, 0, sizeof(buf));
        buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory      = V4L2_MEMORY_MMAP;
        buf.index       = i;
        ESP_ERROR_CHECK (ioctl(sd->cap_fd, VIDIOC_QUERYBUF, &buf));

        sd->cap_buffer[i] = (uint8_t *)mmap(NULL, buf.length, PROT_READ | PROT_WRITE,
                                            MAP_SHARED, sd->cap_fd, buf.m.offset);
        assert(sd->cap_buffer[i]);

        ESP_ERROR_CHECK(ioctl(sd->cap_fd, VIDIOC_QBUF, &buf));
    }

    /* Configure codec output stream */

    memset(&format, 0, sizeof(format));
    format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    format.fmt.pix.width = width;
    format.fmt.pix.height = height;
    format.fmt.pix.pixelformat = capture_fmt;
    ESP_ERROR_CHECK(ioctl(sd->m2m_fd, VIDIOC_S_FMT, &format));

    memset(&req, 0, sizeof(req));
    req.count  = SKIP_STARTUP_FRAME_COUNT;
    req.type   = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    req.memory = V4L2_MEMORY_USERPTR;
    ESP_ERROR_CHECK(ioctl(sd->m2m_fd, VIDIOC_REQBUFS, &req));

    /* Configure codec capture stream */

    memset(&format, 0, sizeof(format));
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.width = width;
    format.fmt.pix.height = height;
    format.fmt.pix.pixelformat = sd->format;
    ESP_ERROR_CHECK(ioctl(sd->m2m_fd, VIDIOC_S_FMT, &format));

    memset(&req, 0, sizeof(req));
    req.count  = 1;
    req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    ESP_ERROR_CHECK(ioctl(sd->m2m_fd, VIDIOC_REQBUFS, &req));

    memset(&buf, 0, sizeof(buf));
    buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory      = V4L2_MEMORY_MMAP;
    buf.index       = 0;
    ESP_ERROR_CHECK (ioctl(sd->m2m_fd, VIDIOC_QUERYBUF, &buf));

    sd->m2m_cap_buffer = (uint8_t *)mmap(NULL, buf.length, PROT_READ | PROT_WRITE,
                                         MAP_SHARED, sd->m2m_fd, buf.m.offset);
    assert(sd->m2m_cap_buffer);

    ESP_ERROR_CHECK(ioctl(sd->m2m_fd, VIDIOC_QBUF, &buf));

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ESP_ERROR_CHECK(ioctl(sd->m2m_fd, VIDIOC_STREAMON, &type));
    type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    ESP_ERROR_CHECK(ioctl(sd->m2m_fd, VIDIOC_STREAMON, &type));

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ESP_ERROR_CHECK(ioctl(sd->cap_fd, VIDIOC_STREAMON, &type));

    /* Skip the first few frames of the image to get a stable image. */
    for (int i = 0; i < SKIP_STARTUP_FRAME_COUNT; i++) {
        memset(&buf, 0, sizeof(buf));
        buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        ESP_ERROR_CHECK(ioctl(sd->cap_fd, VIDIOC_DQBUF, &buf));
        ESP_ERROR_CHECK(ioctl(sd->cap_fd, VIDIOC_QBUF, &buf));
    }

    /* Init sd encoder frame buffer's basic info. */
    sd->sd_fb.width = width;
    sd->sd_fb.height = height;
    sd->sd_fb.fmt = sd->format == V4L2_PIX_FMT_JPEG ? SD_IMG_FORMAT_JPEG : SD_IMG_FORMAT_H264;

    return ESP_OK;
}

static void example_video_stop(device_ctx_t *sd)
{
    int type;

    ESP_LOGD(TAG, "Video stop");

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(sd->cap_fd, VIDIOC_STREAMOFF, &type);

    type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    ioctl(sd->m2m_fd, VIDIOC_STREAMOFF, &type);
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(sd->m2m_fd, VIDIOC_STREAMOFF, &type);
}

static sd_card_fb_t *example_video_fb_get(device_ctx_t *sd)
{
    struct v4l2_buffer cap_buf;
    struct v4l2_buffer m2m_out_buf;
    struct v4l2_buffer m2m_cap_buf;
    int64_t us;

    ESP_LOGD(TAG, "Video get");
    /*
     * V4L2 M2M(Memory to Memory) Workflow:
     *
     **********************   RAW    ********       *************
     * Encoder            * <------- * V4L2 * <---- * Program   *
     * (e.g. jpeg, h.264) * -------> * M2M  * ----> *           *
     **********************  h264    ********       *************
     *
     *The process of obtaining the encoded data can be briefly described as follows:

     **********  DQBUF
     * cap_fd * -------------> Dequeue a filled (capturing) original image data buffer from the cap_fd’s receive queue.
     **********                                                                     |
     *                                                                              |
     *                                                                              v
     *                                                                      QBUF **********
     * Enqueue the original image buf to m2m_fd output buffer queue <----------- * m2m_fd *
     * |                                                                         **********
     * |
     * v
     **********  DQBUF
     * m2m_fd * -------> Dequeue a filled (capturing) buffer to get the encoded image data from m2m_fd's outgoing queue.
     **********
    */
    memset(&cap_buf, 0, sizeof(cap_buf));
    cap_buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    cap_buf.memory = V4L2_MEMORY_MMAP;
    ESP_ERROR_CHECK(ioctl(sd->cap_fd, VIDIOC_DQBUF, &cap_buf));

    memset(&m2m_out_buf, 0, sizeof(m2m_out_buf));
    m2m_out_buf.index  = 0;
    m2m_out_buf.type   = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    m2m_out_buf.memory = V4L2_MEMORY_USERPTR;
    m2m_out_buf.m.userptr = (unsigned long)sd->cap_buffer[cap_buf.index];
    m2m_out_buf.length = cap_buf.bytesused;
    ESP_ERROR_CHECK(ioctl(sd->m2m_fd, VIDIOC_QBUF, &m2m_out_buf));

    memset(&m2m_cap_buf, 0, sizeof(m2m_cap_buf));
    m2m_cap_buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    m2m_cap_buf.memory = V4L2_MEMORY_MMAP;
    ESP_ERROR_CHECK(ioctl(sd->m2m_fd, VIDIOC_DQBUF, &m2m_cap_buf));

    ESP_ERROR_CHECK(ioctl(sd->cap_fd, VIDIOC_QBUF, &cap_buf));
    ESP_ERROR_CHECK(ioctl(sd->m2m_fd, VIDIOC_DQBUF, &m2m_out_buf));
    /* Notes: If multiple m2m buffers are used, 'm2m_cap_buf.index' can be used to get the actual address of buf.
    Here, only one m2m buffer is used, so the address of the m2m buffer is directly referenced.*/
    sd->sd_fb.buf = sd->m2m_cap_buffer;
    sd->sd_fb.buf_bytesused = m2m_cap_buf.bytesused;
    us = esp_timer_get_time();
    sd->sd_fb.timestamp.tv_sec = us / 1000000UL;;
    sd->sd_fb.timestamp.tv_usec = us % 1000000UL;

    return &sd->sd_fb;
}

static void example_video_fb_return(device_ctx_t *sd)
{
    struct v4l2_buffer m2m_cap_buf;

    ESP_LOGD(TAG, "Video return");

    m2m_cap_buf.index  = 0;
    m2m_cap_buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    m2m_cap_buf.memory = V4L2_MEMORY_MMAP;
    ESP_ERROR_CHECK(ioctl(sd->m2m_fd, VIDIOC_QBUF, &m2m_cap_buf));
}
static esp_err_t example_write_file(FILE *f, const uint8_t *data, size_t len)
{
    size_t written;

    do {
        written = fwrite(data, 1, len, f);
        len -= written;
        data += written;
    } while ( written && len );
    fflush(f);

    return ESP_OK;
}

static esp_err_t store_jpg_to_sd_card(device_ctx_t *sd)
{
    printf("store_jpg_to_sd_card\n");
    char string[32] = {0};
    char file_name_str[48] = {0};
    int current_time_us;
    struct timeval current_time;
    sd_card_fb_t *image_fb = NULL;
    FILE *f = NULL;
    struct stat st;
 
    ESP_ERROR_CHECK(example_video_start(sd));

    current_time_us = esp_timer_get_time();
    current_time.tv_sec = current_time_us / 1000000UL;;
    current_time.tv_usec = current_time_us % 1000000UL;
    /* Generate a file name based on the current time */
    itoa((int)current_time.tv_sec, string, 10);
    strcat(strcpy(file_name_str, MOUNT_POINT"/"), string);
    memset(string, 0x0, sizeof(string));
    itoa((int)current_time.tv_usec, string, 10);
    strcat(file_name_str, "_");
    strcat(file_name_str, string);
    strcat(file_name_str, ".jpg");

    ESP_LOGI(TAG, "file name:%s", file_name_str);

    if (stat(file_name_str, &st) == 0) {
        /* Delete it if it exists */
        ESP_LOGW(TAG, "Delete original file %s", file_name_str);
        unlink(file_name_str);
    }

    f = fopen(file_name_str, "wb");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return ESP_FAIL;
    }
    image_fb = example_video_fb_get(sd);
    example_write_file(f, image_fb->buf, image_fb->buf_bytesused);
    example_video_fb_return(sd);
    fclose(f);
    
    example_video_stop(sd);
    ESP_LOGI(TAG, "File written");

    return ESP_OK;
}

void bsp_video_jpg_init(device_ctx_t *video)
{
    ESP_ERROR_CHECK(store_jpg_to_sd_card(video));
 
}
