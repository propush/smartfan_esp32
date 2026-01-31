#include "config.hpp"
#include "esp_log.h"

bool Config::validate() {
    bool valid = true;
    const char* TAG = "config";

    // Validate battery thresholds
    if (Config::LOW_BATTERY_MV >= Config::WARNING_BATTERY_MV) {
        ESP_LOGE(TAG, "LOW_BATTERY_MV (%lu) must be less than WARNING_BATTERY_MV (%lu)",
                 static_cast<unsigned long>(Config::LOW_BATTERY_MV),
                 static_cast<unsigned long>(Config::WARNING_BATTERY_MV));
        valid = false;
    }

    if (Config::WARNING_BATTERY_MV >= Config::FULL_BATTERY_MV) {
        ESP_LOGE(TAG, "WARNING_BATTERY_MV (%lu) must be less than FULL_BATTERY_MV (%lu)",
                 static_cast<unsigned long>(Config::WARNING_BATTERY_MV),
                 static_cast<unsigned long>(Config::FULL_BATTERY_MV));
        valid = false;
    }

    // Validate timing configurations
    if (Config::FAN_ON_TIME_MS == 0) {
        ESP_LOGE(TAG, "FAN_ON_TIME_MS must be greater than 0");
        valid = false;
    }

    if (Config::ADC_INTERVAL_MS == 0) {
        ESP_LOGE(TAG, "ADC_INTERVAL_MS must be greater than 0");
        valid = false;
    }

    if (Config::MAX_PRESS_COUNT == 0) {
        ESP_LOGE(TAG, "MAX_PRESS_COUNT must be greater than 0");
        valid = false;
    }

    if (Config::MAX_FAN_TIME_MS < Config::FAN_ON_TIME_MS) {
        ESP_LOGE(TAG, "MAX_FAN_TIME_MS (%lu) must be greater than or equal to FAN_ON_TIME_MS (%lu)",
                 static_cast<unsigned long>(Config::MAX_FAN_TIME_MS),
                 static_cast<unsigned long>(Config::FAN_ON_TIME_MS));
        valid = false;
    }

    // Validate GPIO configurations
    if (Config::BUTTON_PIN == Config::FAN_GATE_PIN || Config::BUTTON_PIN == Config::LED_PIN || Config::BUTTON_PIN == Config::BATTERY_ADC_PIN) {
        ESP_LOGE(TAG, "BUTTON_PIN must be different from other pins");
        valid = false;
    }

    if (Config::FAN_GATE_PIN == Config::LED_PIN || Config::FAN_GATE_PIN == Config::BATTERY_ADC_PIN) {
        ESP_LOGE(TAG, "FAN_GATE_PIN must be different from other pins");
        valid = false;
    }

    if (Config::LED_PIN == Config::BATTERY_ADC_PIN) {
        ESP_LOGE(TAG, "LED_PIN must be different from BATTERY_ADC_PIN");
        valid = false;
    }

    if (valid) {
        ESP_LOGI(TAG, "Configuration validation passed");
    }

    return valid;
}