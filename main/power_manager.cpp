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
}

namespace power_manager {

void init()
{
    // Configure button pin as RTC GPIO for deep sleep wake
    // ESP32-S3: GPIO4 is RTC_GPIO4
    esp_sleep_enable_ext0_wakeup(BUTTON_PIN, 0);  // Wake on LOW level

    ESP_LOGI(TAG, "Power manager initialized (deep sleep wake on GPIO%d)", BUTTON_PIN);

    // Log wake-up cause
    auto cause = get_wakeup_cause();
    switch (cause) {
        case ESP_SLEEP_WAKEUP_EXT0:
            ESP_LOGI(TAG, "Woke up from button press");
            break;
        case ESP_SLEEP_WAKEUP_TIMER:
            ESP_LOGI(TAG, "Woke up from timer");
            break;
        case ESP_SLEEP_WAKEUP_UNDEFINED:
            ESP_LOGI(TAG, "Power-on reset (not from deep sleep)");
            break;
        default:
            ESP_LOGI(TAG, "Wake cause: %d", cause);
            break;
    }
}

[[noreturn]] void enter_deep_sleep()
{
    ESP_LOGW(TAG, "Entering deep sleep - press button to wake");

    // Isolate GPIO to minimize current draw
    // Keep BUTTON_PIN active for wake-up
    rtc_gpio_isolate(FAN_GATE_PIN);
    rtc_gpio_isolate(BATTERY_ADC_PIN);

    // Small delay to allow log to flush
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
    auto cause = get_wakeup_cause();
    return cause != ESP_SLEEP_WAKEUP_UNDEFINED;
}

} // namespace power_manager
