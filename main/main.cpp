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
    // Configure CPU frequency to 40 MHz for power savings
    esp_pm_config_t pm_config = {
        .max_freq_mhz = 40,
        .min_freq_mhz = 40,
        .light_sleep_enable = false
    };
    esp_err_t pm_err = esp_pm_configure(&pm_config);
    if (pm_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure power management: %s", esp_err_to_name(pm_err));
    }

    ESP_LOGI(TAG, "ESP32-S3 Fan Controller Starting (CPU: 40 MHz)");
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

    // Start all tasks
    ESP_LOGI(TAG, "Starting tasks...");

    fan_controller::start();
    adc_monitor::start();
    button_handler::start();

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "System ready!");
    ESP_LOGI(TAG, "  - Short press: start fan (%lu s), stack up to %lu presses",
             static_cast<unsigned long>(Config::FAN_ON_TIME_MS / 1000),
             static_cast<unsigned long>(Config::MAX_PRESS_COUNT));
    ESP_LOGI(TAG, "  - Long press (>%lu ms): stop fan",
             static_cast<unsigned long>(Config::LONG_PRESS_TIME_MS));
    ESP_LOGI(TAG, "  - LED blinks N times for N presses");
    ESP_LOGI(TAG, "  - Battery monitor: every %lu seconds",
             static_cast<unsigned long>(Config::ADC_INTERVAL_MS / 1000));
    ESP_LOGI(TAG, "  - Low battery cutoff: %lu mV (enters deep sleep)",
             static_cast<unsigned long>(Config::LOW_BATTERY_MV));
    ESP_LOGI(TAG, "========================================");
}
