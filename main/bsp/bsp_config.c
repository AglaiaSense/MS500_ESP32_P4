// FILE: e:/03-MS500-P4/01.code/MS500_ESP32_P4/main/bsp/bsp_config.c

#include "bsp_config.h"

static const char *TAG = "BSP_CONFIG";

// 全局变量声明
device_ctx_t *device_ctx;

// 初始化 uvc_t 结构体
void bsp_struct_alloc(void) {
    device_ctx = calloc(1, sizeof(device_ctx_t));
    if (device_ctx == NULL) {
        // 处理内存分配失败的情况
        ESP_LOGE("BSP_CONFIG", "Failed to allocate memory for uvc_t");
    }
}