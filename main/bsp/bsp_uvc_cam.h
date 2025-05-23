#ifndef BSP_UVC_CAM_H
#define BSP_UVC_CAM_H

#include "bsp_config.h"

#include "esp_err.h"
#include "esp_log.h"

void bsp_uvc_init(device_ctx_t *device_ctx);
void bsp_uvc_deinit(device_ctx_t *uvc);

#endif // BSP_UVC_CAM_H