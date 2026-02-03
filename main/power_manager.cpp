#include "power_manager.hpp"
#include "config.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "esp_log.h"
#include "esp_sleep.h"

namespace {
    const char* TAG = "power_manager";
    bool was_woken_from_deep_sleep = false;
}

namespace power_manager {

esp_err_t init()
{
    // Configure button pin as RTC GPIO for deep sleep wake
    // ESP32-S3: GPIO4 is RTC_GPIO4
    esp_err_t err = esp_sleep_enable_ext0_wakeup(Config::BUTTON_PIN, 0);  // Wake on LOW level
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable deep sleep wake on GPIO%d: %s",
                 Config::BUTTON_PIN, esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "Power manager initialized (deep sleep wake on GPIO%d)", Config::BUTTON_PIN);

    // Log wake-up cause
    auto cause = get_wakeup_cause();
    switch (cause) {
        case ESP_SLEEP_WAKEUP_EXT0:
            ESP_LOGI(TAG, "Woke up from button press");
            was_woken_from_deep_sleep = true;
            break;
        case ESP_SLEEP_WAKEUP_TIMER:
            ESP_LOGI(TAG, "Woke up from timer");
            was_woken_from_deep_sleep = false;
            break;
        case ESP_SLEEP_WAKEUP_UNDEFINED:
            ESP_LOGI(TAG, "Power-on reset (not from deep sleep)");
            was_woken_from_deep_sleep = false;
            break;
        default:
            ESP_LOGI(TAG, "Wake cause: %d", cause);
            was_woken_from_deep_sleep = false;
            break;
    }

    // If waking from deep sleep, restore GPIOs that were isolated
    if (woke_from_deep_sleep()) {
        ESP_LOGI(TAG, "Restoring isolated GPIOs after deep sleep wake");

        // rtc_gpio_isolate() enables hold - we must disable it first
        // Then de-initialize RTC GPIO mode to allow normal GPIO operation
        if (rtc_gpio_is_valid_gpio(Config::FAN_GATE_PIN)) {
            rtc_gpio_hold_dis(Config::FAN_GATE_PIN);
            rtc_gpio_deinit(Config::FAN_GATE_PIN);
        }
        if (rtc_gpio_is_valid_gpio(Config::LED_PIN)) {
            rtc_gpio_hold_dis(Config::LED_PIN);
            rtc_gpio_deinit(Config::LED_PIN);
        }
        if (rtc_gpio_is_valid_gpio(Config::BATTERY_ADC_PIN)) {
            rtc_gpio_hold_dis(Config::BATTERY_ADC_PIN);
            rtc_gpio_deinit(Config::BATTERY_ADC_PIN);
        }
    }

    return ESP_OK;
}

[[noreturn]] void enter_deep_sleep()
{
    ESP_LOGW(TAG, "Preparing for deep sleep...");

    // EXT0 is level-triggered: if button is LOW when we sleep, we wake immediately.
    // Wait for button release before sleeping.
    if (gpio_get_level(Config::BUTTON_PIN) == 0) {
        ESP_LOGI(TAG, "Waiting for button release before sleep...");
        while (gpio_get_level(Config::BUTTON_PIN) == 0) {
            vTaskDelay(pdMS_TO_TICKS(50));
        }
        // Debounce delay after release
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    ESP_LOGW(TAG, "Entering deep sleep - press button to wake");

    // Ensure outputs are LOW before isolating
    gpio_set_level(Config::FAN_GATE_PIN, 0);
    gpio_set_level(Config::LED_PIN, 0);

    // Isolate all output GPIOs to minimize current draw
    // This puts them in high-impedance state during sleep
    // Keep BUTTON_PIN active for wake-up (not isolated)
    if (rtc_gpio_is_valid_gpio(Config::FAN_GATE_PIN)) {
        rtc_gpio_isolate(Config::FAN_GATE_PIN);
    }
    if (rtc_gpio_is_valid_gpio(Config::LED_PIN)) {
        rtc_gpio_isolate(Config::LED_PIN);
    }
    if (rtc_gpio_is_valid_gpio(Config::BATTERY_ADC_PIN)) {
        rtc_gpio_isolate(Config::BATTERY_ADC_PIN);
    }

    // Configure RTC pull-up on button pin - regular GPIO pull-up is lost during deep sleep
    rtc_gpio_init(Config::BUTTON_PIN);
    rtc_gpio_set_direction(Config::BUTTON_PIN, RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_pullup_en(Config::BUTTON_PIN);
    rtc_gpio_pulldown_dis(Config::BUTTON_PIN);

    // Small delay to allow log to flush and pull-up to stabilize
    vTaskDelay(pdMS_TO_TICKS(100));

    // Enter deep sleep - will wake on button press (GPIO4 LOW)
    esp_deep_sleep_start();

    // Never reached
    while (true) {}
}

esp_sleep_wakeup_cause_t get_wakeup_cause()
{
    return esp_sleep_get_wakeup_cause();
}

bool woke_from_deep_sleep()
{
    return was_woken_from_deep_sleep;
}

} // namespace power_manager
