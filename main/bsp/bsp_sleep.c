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

    ESP_LOGI(TAG, "-----------------------------------");
}

// 配置唤醒GPIO
void configure_wakeup_gpio(gpio_num_t gpio_num, esp_sleep_ext1_wakeup_mode_t level_mode) {
    //  唤醒
    esp_sleep_enable_ext1_wakeup(gpio_num, level_mode);
}

// 配置唤醒时间
void configure_wakeup_timer(uint64_t sleep_time_us) {

    // 增加0.5秒，不然在1s内会重复睡眠
    sleep_time_us = sleep_time_us + SEC_TO_USEC(0.5);

    // 配置计时器唤醒
    ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(sleep_time_us));
}

// --------------------------   进入睡眠 ------------------------------------------
void enter_light_sleep(uint64_t sleep_time_us, gpio_num_t gpio_num, esp_sleep_ext1_wakeup_mode_t level_mode) {

    // 从睡眠中唤醒后，会继续执行下面的代码
    ESP_LOGI(TAG, "==================================================================");
    ESP_LOGI(TAG, "light sleep");

    // 配置唤醒源
    if (sleep_time_us > 0) {
        configure_wakeup_timer(sleep_time_us);
    }
    if (gpio_num != GPIO_NUM_NC) {
        configure_wakeup_gpio(gpio_num, level_mode);
    }

    // 开始进入轻睡眠模式
    esp_light_sleep_start();

    // 检查唤醒原因
    check_wakeup_reason();
}

void enter_deep_sleep(uint64_t sleep_time_us, gpio_num_t gpio_num, esp_sleep_ext1_wakeup_mode_t level_mode) {

    // 从睡眠中唤醒后，会继续执行下面的代码
    ESP_LOGI(TAG, "==================================================================");
    ESP_LOGI(TAG, "deep sleep");

    // 配置唤醒源
    if (sleep_time_us > 0) {
        configure_wakeup_timer(sleep_time_us);
    }
    if (gpio_num != GPIO_NUM_NC) {
        configure_wakeup_gpio(gpio_num, level_mode);
    }

    // 进入深度睡眠模式
    esp_deep_sleep_start();

    // 检查唤醒原因
    check_wakeup_reason();
}

// 进入轻睡眠模式
void enter_light_sleep_time(uint64_t sleep_time_us) {
    enter_light_sleep(sleep_time_us, GPIO_NUM_NC, -1);
}

// 进入深度睡眠模式
void enter_deep_sleep_time(uint64_t sleep_time_us) {
    enter_deep_sleep(sleep_time_us, GPIO_NUM_NC, -1);
}

void gpio_wakeup_init(void) {
    /* Initialize GPIO */
    gpio_config_t config = {
        .pin_bit_mask = GPIO_INPUT_PIN_SEL,
        .mode = GPIO_MODE_INPUT,
        .pull_down_en = false,
        .pull_up_en = false,
        .intr_type = GPIO_INTR_DISABLE};

    gpio_config(&config);
}

void enter_light_sleep_gpio() {
    gpio_wakeup_init();

    /* Enable wake up from GPIO */
    gpio_wakeup_enable(GPIO_INPUT_WAKEUP, GPIO_INPUT_WAKEUP_LEVEL == 0 ? GPIO_INTR_LOW_LEVEL : GPIO_INTR_HIGH_LEVEL);
    esp_sleep_enable_gpio_wakeup();


    esp_light_sleep_start();
};

// 进入深度睡眠模式
void enter_deep_sleep_gpio() {
    enter_deep_sleep(-1, GPIO_NUM_45, ESP_GPIO_WAKEUP_GPIO_HIGH);
}
