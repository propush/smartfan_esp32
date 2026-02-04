#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_pm.h"
#include "nvs_flash.h"

#include "config.hpp"
#include "power_manager.hpp"
#include "adc_monitor.hpp"
#include "button_handler.hpp"
#include "fan_controller.hpp"
#include "led_controller.hpp"

namespace {
    const char* TAG = "main";
}

extern "C" void app_main()
{
    // Configure CPU frequency to 20 MHz for power savings
    esp_pm_config_t pm_config = {
        .max_freq_mhz = 20,
        .min_freq_mhz = 20,
        .light_sleep_enable = false
    };
    esp_err_t pm_err = esp_pm_configure(&pm_config);
    if (pm_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure power management: %s", esp_err_to_name(pm_err));
    }

    ESP_LOGI(TAG, "ESP32-S3 Fan Controller Starting (CPU: 20 MHz)");
    ESP_LOGI(TAG, "========================================");

    // Validate configuration
    if (!Config::validate()) {
        ESP_LOGE(TAG, "Configuration validation failed!");
        // Continue execution but log error
    }

    // Initialize power manager (sets up deep sleep wake source)
    esp_err_t err = power_manager::init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize power manager: %s", esp_err_to_name(err));
        // Continue execution but log error
    }

    // Initialize NVS (required by some ESP-IDF components)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize all subsystems
    ESP_LOGI(TAG, "Initializing subsystems...");

    err = fan_controller::init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize fan controller: %s", esp_err_to_name(err));
        // Continue execution but log error
    }

    err = led_controller::init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize LED controller: %s", esp_err_to_name(err));
        // Continue execution but log error
    }

    err = adc_monitor::init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize ADC monitor: %s", esp_err_to_name(err));
        // Continue execution but log error
    }

    err = button_handler::init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize button handler: %s", esp_err_to_name(err));
        // Continue execution but log error
    }

    // Log initial battery voltage
    uint32_t voltage = adc_monitor::read_voltage_mv();
    ESP_LOGI(TAG, "Initial battery voltage: %lu mV", static_cast<unsigned long>(voltage));

    // Check battery on startup
    if (voltage < Config::LOW_BATTERY_MV && voltage > 0) {
        ESP_LOGW(TAG, "Battery voltage too low for operation!");
        ESP_LOGW(TAG, "Entering deep sleep to protect battery...");
        vTaskDelay(pdMS_TO_TICKS(500));
        power_manager::enter_deep_sleep();
    }

    // Check wake cause and decide flow
    bool start_fan_immediately = false;
    bool from_deep_sleep = power_manager::woke_from_deep_sleep();
    ESP_LOGI(TAG, "Wake check: from_deep_sleep=%d", from_deep_sleep);

    if (from_deep_sleep) {
        // Woke from button press - start fan
        ESP_LOGI(TAG, "Button wake detected - will start fan");
        start_fan_immediately = true;
    } else {
        // Fresh power-on - check if button is being held
        // Allow GPIO to stabilize with pull-up before reading
        vTaskDelay(pdMS_TO_TICKS(50));

        // Sample button multiple times to debounce
        int pressed_count = 0;
        for (int i = 0; i < 5; i++) {
            if (button_handler::is_pressed()) {
                pressed_count++;
            }
            vTaskDelay(pdMS_TO_TICKS(10));
        }

        ESP_LOGI(TAG, "Button check: %d/5 samples pressed", pressed_count);

        if (pressed_count >= 3) {
            ESP_LOGI(TAG, "Button held during power-on - starting fan");
            start_fan_immediately = true;
        } else {
            ESP_LOGI(TAG, "Power-on reset - entering deep sleep");
            vTaskDelay(pdMS_TO_TICKS(100)); // Allow log to flush
            power_manager::enter_deep_sleep();
        }
    }

    // Start all tasks
    ESP_LOGI(TAG, "Starting tasks...");

    fan_controller::start();
    adc_monitor::start();
    button_handler::start();

    if (start_fan_immediately) {
        ESP_LOGI(TAG, "Auto-starting fan from button wake");
        fan_controller::on();  // Direct call, bypasses queue
    }

    ESP_LOGI(TAG, "System running - fan will auto-sleep when off");

    // Supervisor loop: wait for fan to turn off, then enter deep sleep
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(200));  // Check every 200ms

        FanState state = fan_controller::get_state();

        if (state == FanState::Off) {
            ESP_LOGI(TAG, "Fan off - entering deep sleep");
            vTaskDelay(pdMS_TO_TICKS(100)); // Brief delay for pending operations
            power_manager::enter_deep_sleep();
        }

        if (state == FanState::Shutdown) {
            // Low battery - adc_monitor handles sleep
            break;
        }
    }
}
