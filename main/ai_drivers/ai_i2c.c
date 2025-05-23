#include "ai_i2c.h"
#include "esp_log.h"

static const char *TAG = "AI_I2C";

void ai_i2c_master_init(void) {

    esp_err_t ret; // 用于存储函数返回的错误码

    int i2c_master_port = I2C_MASTER_NUM; // 获取I2C主设备端口号

    // I2C配置结构体初始化
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,                // 设置为主模式
        .sda_io_num = I2C_MASTER_SDA_IO,        // 设置SDA引脚
        .scl_io_num = I2C_MASTER_SCL_IO,        // 设置SCL引脚
        .sda_pullup_en = GPIO_PULLUP_ENABLE,    // 使能SDA上拉电阻
        .scl_pullup_en = GPIO_PULLUP_ENABLE,    // 使能SCL上拉电阻
        .master.clk_speed = I2C_MASTER_FREQ_HZ, // 设置I2C时钟频率
    };

    // 配置I2C参数
    ret = i2c_param_config(i2c_master_port, &conf);
    ESP_ERROR_CHECK(ret); // 检查初始化结果，如果失败则终止程序

    // 安装I2C驱动
    ret = i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
    ESP_ERROR_CHECK(ret); // 检查安装结果，如果失败则终止程序

    // 打印I2C初始化成功日志
    ESP_LOGI(TAG, "I2C initialized successfully");
}

/**
 * @brief i2c uninitialization
 */
void ai_i2c_master_uninit(void) {
    esp_err_t err = i2c_driver_delete(I2C_MASTER_NUM);
    ESP_ERROR_CHECK(err);
    ESP_LOGI(TAG, "I2C de-initialized successfully");
}

esp_err_t ai_i2c_register_read(uint16_t reg_addr, size_t *data, size_t size) {
    esp_err_t err;                                                    // 用于存储I2C操作返回的错误码
    uint8_t p_buf[4];                                                 // 用于存储从I2C设备读取的原始数据
    uint8_t write_buf[2] = {(reg_addr >> 8) & 0xff, reg_addr & 0xff}; // 将16位寄存器地址拆分为高8位和低8位

    // 通过I2C从传感器读取数据
    err = i2c_master_write_read_device(I2C_MASTER_NUM, IMX501_SENSOR_ADDR, write_buf, sizeof(write_buf), p_buf, size, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);

    *data = 0; // 初始化输出数据
    // 将读取的字节数据按顺序组合为完整的数据
    for (int i = 0; i < size; i++) {
        *data += (size_t)(p_buf[i] << ((size - 1 - i) * 8));
    }

    return err; // 返回I2C操作结果
}

esp_err_t ai_i2c_register_read_id(uint16_t reg_addr, uint8_t  *data, size_t size) {
    esp_err_t err;
    uint8_t write_buf[2] = {(reg_addr >> 8) & 0xff, reg_addr & 0xff};
    // 通过I2C从传感器读取ID数据
    err = i2c_master_write_read_device(I2C_MASTER_NUM, IMX501_SENSOR_ADDR, write_buf, sizeof(write_buf), data, size, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);

    return err;
}

esp_err_t ai_i2c_register_write(uint16_t reg_addr, size_t data, size_t size) {
    int ret;                                                                      // 用于存储I2C操作返回的错误码
    uint8_t write_buf[6] = {(reg_addr >> 8) & 0xff, reg_addr & 0xff, 0, 0, 0, 0}; // 初始化写入缓冲区，包含寄存器地址和数据

    // 打印寄存器写入日志，显示寄存器地址和写入的数据
    // ESP_LOGI(TAG, "addr: 0x%04X, data: 0x%08X", reg_addr, data);

    // 根据数据大小将数据拆分为字节并填充到缓冲区
    if (size == 1) {
        write_buf[2] = data & 0xff; // 写入1字节数据
    } else if (size == 2) {
        write_buf[2] = (data >> 8) & 0xff; // 写入高字节
        write_buf[3] = data & 0xff;        // 写入低字节
    } else if (size == 4) {
        write_buf[2] = (data >> 24) & 0xff; // 写入最高字节
        write_buf[3] = (data >> 16) & 0xff; // 写入次高字节
        write_buf[4] = (data >> 8) & 0xff;  // 写入次低字节
        write_buf[5] = data & 0xff;         // 写入最低字节
    }

    // 通过I2C向设备写入数据
    ret = i2c_master_write_to_device(I2C_MASTER_NUM, IMX501_SENSOR_ADDR, write_buf, size + 2, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);

    return ret; // 返回I2C操作结果
}