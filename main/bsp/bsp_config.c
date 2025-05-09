// FILE: e:/03-MS500-P4/01.code/MS500_ESP32_P4/main/bsp/bsp_config.c

#include "bsp_config.h"

static const char *TAG = "BSP_CONFIG";
extern device_ctx_t *device_ctx;


void delay_ms(int nms) {
    vTaskDelay(pdMS_TO_TICKS(nms));
}
