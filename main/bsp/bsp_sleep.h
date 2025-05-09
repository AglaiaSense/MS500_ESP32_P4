#ifndef BSP_SLEEP_H
#define BSP_SLEEP_H

#include "bsp_config.h"

#define GPIO_INPUT_WAKEUP    (45)
#define GPIO_INPUT_WAKEUP_LEVEL (1)
#define GPIO_INPUT_PIN_SEL  ((1ULL<<GPIO_INPUT_WAKEUP))

void enter_light_sleep_time(uint64_t sleep_time_us) ;
void enter_deep_sleep_time(uint64_t sleep_time_us) ;

void enter_light_sleep_gpio() ;
void enter_deep_sleep_gpio() ;

void configure_gpio_wakeup(void) ;

#endif /* BSP_SLEEP_H */
