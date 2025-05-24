#include "ai_i2c.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

static const char *TAG = "AI_I2C";

// 新增全局句柄
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

esp_err_t ai_i2c_register_read(uint16_t reg_addr, size_t *data, size_t size) {
    uint8_t write_buf[2] = {(reg_addr >> 8) & 0xff, reg_addr & 0xff};
    uint8_t read_buf[4] = {0};

    // 创建传输描述符
    i2c_master_transmit_receive(
        dev_handle,
        write_buf, sizeof(write_buf),
        read_buf, size,
        I2C_MASTER_TIMEOUT_MS);

    *data = 0;
    for (int i = 0; i < size; i++) {
        *data |= (size_t)(read_buf[i] << ((size - 1 - i) * 8));
    }
    return ESP_OK;
}

esp_err_t ai_i2c_register_read_id(uint16_t reg_addr, uint8_t *data, size_t size) {
    uint8_t write_buf[2] = {(reg_addr >> 8) & 0xff, reg_addr & 0xff};

    return i2c_master_transmit_receive(
        dev_handle,
        write_buf, sizeof(write_buf),
        data, size,
        I2C_MASTER_TIMEOUT_MS);
}

esp_err_t ai_i2c_register_write(uint16_t reg_addr, size_t data, size_t size) {
    uint8_t write_buf[6] = {
        (reg_addr >> 8) & 0xff,
        reg_addr & 0xff};

    // 数据打包
    for (int i = 0; i < size; i++) {
        write_buf[2 + i] = (data >> (8 * (size - 1 - i))) & 0xff;
    }

    return i2c_master_transmit(
        dev_handle,
        write_buf, 2 + size,
        I2C_MASTER_TIMEOUT_MS);
}