#include "ai_i2c.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "esp_video_device_internal.h"

static const char *TAG = "AI_I2C";

#if 0
static i2c_master_bus_handle_t bus_handle = NULL;
static i2c_master_dev_handle_t dev_handle = NULL;

void ai_i2c_master_init(void) {
    esp_err_t ret;

    // 总线配置
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_MASTER_NUM,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,   // 使用默认时钟源
        .glitch_ignore_cnt = 7,              // 滤波设置
        .flags.enable_internal_pullup = true // 启用内部上拉
    };

    // 初始化总线
    ret = i2c_new_master_bus(&bus_config, &bus_handle);
    ESP_ERROR_CHECK(ret);

    // 设备配置
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = IMX501_SENSOR_ADDR,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };

    // 添加设备
    ret = i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle);
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "I2C new driver initialized");
}

void ai_i2c_master_uninit(void) {
    if (dev_handle) {
        ESP_ERROR_CHECK(i2c_master_bus_rm_device(dev_handle));
    }
    if (bus_handle) {
        ESP_ERROR_CHECK(i2c_del_master_bus(bus_handle));
    }
    ESP_LOGI(TAG, "I2C de-initialized successfully");
}
#endif
static esp_sccb_io_handle_t sccb_handle = NULL;

void ai_i2c_copy_sccb_handle(void) {
    printf("%s(%d) \n", __func__, __LINE__);

    esp_cam_sensor_device_t *cam_dev = esp_video_get_csi_video_device_sensor();
    if (cam_dev && cam_dev->sccb_handle) {
        sccb_handle = cam_dev->sccb_handle;
    }else {
        ESP_LOGE(TAG, "Failed to get camera sensor device");
    }
}
esp_err_t ai_i2c_register_read(uint16_t reg_addr, size_t *data, size_t size) {
    if (!sccb_handle || !data || (size != 1 && size != 2 && size != 4)) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t temp;
    *data = 0;

    for (size_t i = 0; i < size; i++) {
        esp_err_t ret = esp_sccb_transmit_receive_reg_a16v8(sccb_handle, reg_addr + i, &temp);
        if (ret != ESP_OK) {
            return ret;
        }
        *data |= ((size_t)temp << ((size - 1 - i) * 8));
    }

    return ESP_OK;
}


esp_err_t ai_i2c_register_read_id(uint16_t reg_addr, uint8_t *data, size_t size) {
    if (!sccb_handle || !data || size == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    for (size_t i = 0; i < size; i++) {
        esp_err_t ret = esp_sccb_transmit_receive_reg_a16v8(sccb_handle, reg_addr + i, &data[i]);
        if (ret != ESP_OK) {
            return ret;
        }
    }

    return ESP_OK;
}

esp_err_t ai_i2c_register_write(uint16_t reg_addr, size_t data, size_t size) {
    if (!sccb_handle || (size != 1 && size != 2 && size != 4)) {
        return ESP_ERR_INVALID_ARG;
    }

    for (size_t i = 0; i < size; i++) {
        uint8_t byte = (data >> ((size - 1 - i) * 8)) & 0xFF;
        esp_err_t ret = esp_sccb_transmit_reg_a16v8(sccb_handle, reg_addr + i, byte);
        if (ret != ESP_OK) {
            return ret;
        }
    }

    return ESP_OK;
}
