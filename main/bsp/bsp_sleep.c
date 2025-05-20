#include "bsp_sleep.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "esp_sleep.h"

#include "bsp_rtc.h"

static const char *TAG = "BSP_SLEEP";
// --------------------------   睡眠 ------------------------------------------

// 检查重启原因
void check_wakeup_reason() {
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

    switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_TIMER:
        ESP_LOGI(TAG, "Wake up from timer");
        break;
    case ESP_SLEEP_WAKEUP_UNDEFINED:
        ESP_LOGI(TAG, "Wake up from GPIO ");
        break;
    default:
        ESP_LOGI(TAG, "Not a deep sleep reset");
        break;
    }
}

void configure_wakeup_gpio(void) {
    /* Initialize GPIO */
    gpio_config_t config = {
        .pin_bit_mask = (1ULL << GPIO_NUM_45),
        .mode = GPIO_MODE_INPUT,
        .pull_down_en = true,
        .pull_up_en = false,
        .intr_type = GPIO_INTR_DISABLE};

    gpio_config(&config);

    gpio_wakeup_enable(GPIO_NUM_45, GPIO_INTR_HIGH_LEVEL);
    // esp_sleep_enable_ext1_wakeup(gpio_num, level_mode);

    esp_sleep_enable_gpio_wakeup();
}

// 配置唤醒时间
void configure_wakeup_timer(uint64_t sleep_time_us) {

    // 增加0.5秒，不然在1s内会重复睡眠
    sleep_time_us = sleep_time_us + SEC_TO_USEC(0.5);

    // 配置计时器唤醒
    ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(sleep_time_us));
}

// --------------------------   深度睡眠 ------------------------------------------

void enter_deep_sleep_time(uint64_t sleep_time_us) {

    ESP_LOGI(TAG, "deep sleep time wakeup");

    // 配置唤醒源
    configure_wakeup_timer(sleep_time_us);

    // 进入深度睡眠模式
    esp_deep_sleep_start();

    // 检查唤醒原因
    check_wakeup_reason();
}

// 进入深度睡眠模式
void enter_deep_sleep_gpio() {

    ESP_LOGI(TAG, "deep sleep gpio wakeup");

    configure_wakeup_gpio();

    // 开始进入轻睡眠模式
    esp_deep_sleep_start();

    // 检查唤醒原因
    check_wakeup_reason();
}

// --------------------------   浅睡眠 ------------------------------------------

void enter_light_sleep_time(uint64_t sleep_time_us) {

    ESP_LOGI(TAG, "light sleep time wakeup");

    // 配置唤醒源
    configure_wakeup_timer(sleep_time_us);

    // 进入深度睡眠模式
    esp_light_sleep_start();

    // 检查唤醒原因
    check_wakeup_reason();
}

// 进入深度睡眠模式
void enter_light_sleep_gpio() {

    ESP_LOGI(TAG, "light sleep gpio wakeup");

    configure_wakeup_gpio();

    // 开始进入轻睡眠模式
    esp_light_sleep_start();

    // 检查唤醒原因
    check_wakeup_reason();
}
