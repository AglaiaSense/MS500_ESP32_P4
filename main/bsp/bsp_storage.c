#include "bsp_storage.h"
#include "esp_log.h"
#include <stdio.h>
#include <sys/stat.h>

static const char *TAG = "BSP_STORAGE";
// ----------------------------------  JPG----------------------------------

static esp_err_t example_write_file(FILE *f, const uint8_t *data, size_t len) {
    size_t written; // 记录每次写入的字节数

    // 循环写入数据，直到所有数据写入完成或写入失败
    do {
        // 将数据写入文件，每次最多写入len字节
        written = fwrite(data, 1, len, f);

        // 如果写入失败（written为0），立即返回错误
        if (written == 0) {
            ESP_LOGE(TAG, "Failed to write data to file");
            return ESP_FAIL;
        }

        // 更新剩余需要写入的字节数
        len -= written;
        // 移动数据指针到未写入的位置
        data += written;
    } while (written && len); // 当有数据写入且还有剩余数据时继续循环

    // 刷新文件缓冲区，确保数据写入存储设备
    fflush(f);

    // 返回成功状态
    return ESP_OK;
}


/**
 * @brief 获取JPG文件路径，每次调用会自动+1
 * @param file_path 用于存储生成的文件路径
 * @param file_path_size 文件路径缓冲区大小
 * @return esp_err_t 返回操作结果
 */
esp_err_t get_jpg_file_path(char *file_path, size_t file_path_size) {
    FILE *f = NULL;
    int file_count = 0;
    const int numMax = 3000;  // 最大文件数
    const int numSort = 500;  // 每个目录的文件数

    // 打开或创建list_jpg.txt文件
    const char *list_file = MOUNT_POINT "/list_jpg.txt";
    f = fopen(list_file, "r");
    if (f != NULL) {
        // 读取文件中的数字
        if (fscanf(f, "%d", &file_count) != 1) {
            file_count = 0;
        }
        fclose(f);
    }

    // 文件计数加1，并检查最大值
    file_count++;
    if (file_count >= numMax || file_count <= 0) {
        file_count = 1;
    }

    // 更新list_jpg.txt文件
    f = fopen(list_file, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open list file for writing");
        return ESP_FAIL;
    }
    fprintf(f, "%d", file_count);
    fclose(f);

    // 计算文件夹编号
    int folder_index = (file_count - 1) / numSort + 1;

    // 生成文件夹路径
    char folder_path[48];
    snprintf(folder_path, sizeof(folder_path), "%s/jpg%d", MOUNT_POINT, folder_index);

    // 创建文件夹（如果不存在）
    struct stat st;
    if (stat(folder_path, &st) == -1) {
        mkdir(folder_path, 0777);
    }

    // 生成完整文件路径
    snprintf(file_path, file_path_size, "%s/%d.jpg", folder_path, file_count);

    return ESP_OK;
}
esp_err_t store_jpg_to_sd_card(device_ctx_t *sd) {
    char file_name_str[48] = {0};
    FILE *f = NULL;
    struct stat st;

    // 获取文件路径
    esp_err_t ret = get_jpg_file_path(file_name_str, sizeof(file_name_str));
    if (ret != ESP_OK) {
        return ret;
    }

    // 检查文件是否已存在
    if (stat(file_name_str, &st) == 0) {
        ESP_LOGW(TAG, "Delete original file %s", file_name_str);
        unlink(file_name_str);
    }

    // 以二进制写入模式打开文件
    f = fopen(file_name_str, "wb");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return ESP_FAIL;
    }

    // 将图像数据写入文件
    example_write_file(f, sd->fb.buf, sd->fb.len);
    fclose(f);


    return ESP_OK;
}