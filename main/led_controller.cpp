#include "led_controller.hpp"
#include "config.hpp"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace {
    const char* TAG = "led_controller";
}

namespace led_controller {

void init()
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    gpio_set_level(LED_PIN, 0);

    ESP_LOGI(TAG, "LED controller initialized on GPIO%d", LED_PIN);
}

void blink(uint8_t count)
{
    if (count == 0) {
        return;
    }

    ESP_LOGI(TAG, "Blinking LED %u time(s)", count);

    for (uint8_t i = 0; i < count; i++) {
        gpio_set_level(LED_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(LED_BLINK_ON_MS));
        gpio_set_level(LED_PIN, 0);

        // Add off-delay between blinks (but not after last blink)
        if (i < count - 1) {
            vTaskDelay(pdMS_TO_TICKS(LED_BLINK_OFF_MS));
        }
    }
}

void on()
{
    gpio_set_level(LED_PIN, 1);
}

void off()
{
    gpio_set_level(LED_PIN, 0);
}

} // namespace led_controller
