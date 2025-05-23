/*
 * 2024 Guangshi (Shanghai) CO LTD
 *
 * Wing Hu
 */

#ifndef __FW_LOADER_H__
#define __FW_LOADER_H__


/* download firmware */
static const char *IMAGE_FILE_LOADER = "/spiffs/firmware/loader.fpk";
static const char *IMAGE_FILE_MAINFW = "/spiffs/firmware/firmware.fpk";

// 读写fpk的内存大小
#define IMAGE_FILE_MEMORY_SIZE      (40960)     // 40 * 1024


void fw_init(void);
void fw_uninit(void);

int fw_spi_boot(void);

int fw_flash_update(void);
int fw_flash_boot(void);

#endif
