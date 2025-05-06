#ifndef _BSP_UVC_JPG_H_
#define _BSP_UVC_JPG_H_

#include "bsp_config.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化USB摄像头2
 * @return ESP_OK 成功，其他值失败
 */
esp_err_t usb_cam2_init(void);

#ifdef __cplusplus
}
#endif

#endif /* _BSP_UVC_JPG_H_ */
