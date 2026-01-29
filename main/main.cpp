#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "config.hpp"
#include "power_manager.hpp"
#include "adc_monitor.hpp"
#include "button_handler.hpp"
#include "fan_controller.hpp"

namespace {
    const char* TAG = "main";
}

extern "C" void app_main()
{
    ESP_LOGI(TAG, "ESP32-S3 Fan Controller Starting");
    ESP_LOGI(TAG, "========================================");

    // Initialize power manager (sets up deep sleep wake source)
    power_manager::init();

    // Initialize NVS (required by some ESP-IDF components)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize all subsystems
    ESP_LOGI(TAG, "Initializing subsystems...");

    fan_controller::init();
    adc_monitor::init();
    button_handler::init();

    // Log initial battery voltage
    uint32_t voltage = adc_monitor::read_voltage_mv();
    ESP_LOGI(TAG, "Initial battery voltage: %lu mV", static_cast<unsigned long>(voltage));

    // Check battery on startup
    if (voltage < LOW_BATTERY_MV && voltage > 0) {
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
    ESP_LOGI(TAG, "  - Press button (GPIO%d) to run fan for %lu seconds",
             BUTTON_PIN, static_cast<unsigned long>(FAN_ON_TIME_MS / 1000));
    ESP_LOGI(TAG, "  - Battery monitor: every %lu seconds",
             static_cast<unsigned long>(ADC_INTERVAL_MS / 1000));
    ESP_LOGI(TAG, "  - Low battery cutoff: %lu mV (enters deep sleep)",
             static_cast<unsigned long>(LOW_BATTERY_MV));
    ESP_LOGI(TAG, "========================================");
}
