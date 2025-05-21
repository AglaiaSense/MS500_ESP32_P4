/*
 * 2024 Guangshi (Shanghai) CO LTD
 *
 * Wing Hu
 */

#ifndef __GPIO_H__
#define __GPIO_H__

/*!< 传感器硬件复位引脚     ESP32的GPIO21，低电平复位，高电平正常工作 */
#define GPIO_OUTPUT_IO_RST (2)

 

#define GPIO_OUTPUT_PIN_SEL ((1ULL << GPIO_OUTPUT_IO_RST))

void ai_gpio_init(void);

#endif