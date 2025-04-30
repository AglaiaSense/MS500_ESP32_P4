#ifndef BSP_SD_CARD_H
#define BSP_SD_CARD_H
#include "bsp_config.h"

void bsp_deinit_sd_card(device_ctx_t *sd) ;
esp_err_t bsp_init_sd_card(device_ctx_t *sd) ;

void bsp_sd_card_test(device_ctx_t *sd) ;

 esp_err_t store_jpg_to_sd_card(device_ctx_t *sd);


#endif // BSP_SD_CARD_H
