#ifndef BSP_STORAGE_H
#define BSP_STORAGE_H

#include "bsp_config.h"
#include "bsp_sd_card.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 存储JPG图片到SD卡
 * @param sd 设备上下文指针
 * @return esp_err_t 返回操作结果
 */
esp_err_t store_jpg_to_sd_card(device_ctx_t *sd);

#ifdef __cplusplus
}
#endif

#endif // BSP_STORAGE_H
