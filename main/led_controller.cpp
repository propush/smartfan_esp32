#include "led_controller.hpp"
#include "config.hpp"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace {
    const char* TAG = "led_controller";
    bool initialized = false;
}

namespace led_controller {

esp_err_t init()
{
    if (initialized) {
        ESP_LOGW(TAG, "LED controller already initialized");
        return ESP_OK;
    }

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << Config::LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure LED GPIO: %s", esp_err_to_name(err));
        return err;
    }

    gpio_set_level(Config::LED_PIN, 0);
    initialized = true;

    ESP_LOGI(TAG, "LED controller initialized on GPIO%d", Config::LED_PIN);
    return ESP_OK;
}

void blink(uint8_t count)
{
    if (!initialized) {
        ESP_LOGE(TAG, "LED controller not initialized");
        return;
    }

    if (count == 0) {
        return;
    }

    ESP_LOGI(TAG, "Blinking LED %u time(s)", count);

    for (uint8_t i = 0; i < count; i++) {
        gpio_set_level(Config::LED_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(Config::LED_BLINK_ON_MS));
        gpio_set_level(Config::LED_PIN, 0);

        // Add off-delay between blinks (but not after last blink)
        if (i < count - 1) {
            vTaskDelay(pdMS_TO_TICKS(Config::LED_BLINK_OFF_MS));
        }
    }
}

void blink_low_battery()
{
    if (!initialized) {
        ESP_LOGE(TAG, "LED controller not initialized");
        return;
    }

    ESP_LOGI(TAG, "Blinking low battery warning pattern (5 quick blinks)");

    // 5 quick blinks (150ms on, 150ms off)
    for (int i = 0; i < 5; i++) {
        gpio_set_level(Config::LED_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(Config::LED_BLINK_ON_MS));
        gpio_set_level(Config::LED_PIN, 0);

        // Add off-delay between blinks (but not after last blink)
        if (i < 4) {
            vTaskDelay(pdMS_TO_TICKS(Config::LED_BLINK_OFF_MS));
        }
    }

    // Add a longer pause after the warning pattern
    vTaskDelay(pdMS_TO_TICKS(1000));
}

void on()
{
    if (!initialized) {
        ESP_LOGE(TAG, "LED controller not initialized");
        return;
    }

    gpio_set_level(Config::LED_PIN, 1);
}

void off()
{
    if (!initialized) {
        ESP_LOGE(TAG, "LED controller not initialized");
        return;
    }

    gpio_set_level(Config::LED_PIN, 0);
}

} // namespace led_controller
