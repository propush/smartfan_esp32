#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include <cstdint>

/**
 * @brief Configuration class to manage all system constants
 *
 * This class centralizes all hardware and timing configuration parameters
 * for the ESP32-S3 Fan Controller project. It provides compile-time constants
 * that define pin assignments, timing values, and system thresholds.
 */
class Config {
public:
    // Pin Assignments
    static constexpr gpio_num_t BUTTON_PIN = GPIO_NUM_4;      /**< Button input, internal pull-up, active LOW */
    static constexpr gpio_num_t FAN_GATE_PIN = GPIO_NUM_5;    /**< Fan MOSFET gate, push-pull, default LOW */
    static constexpr gpio_num_t BATTERY_ADC_PIN = GPIO_NUM_6; /**< Battery ADC, ADC1_CH5 */
    static constexpr gpio_num_t LED_PIN = GPIO_NUM_7;         /**< Status LED output */

    // ADC Configuration
    static constexpr adc_atten_t ADC_ATTEN = ADC_ATTEN_DB_12;           /**< 0-2.5V range (approximately) */
    static constexpr adc_bitwidth_t ADC_WIDTH = ADC_BITWIDTH_12;        /**< 12-bit resolution (0-4095) */
    static constexpr adc_channel_t BATTERY_ADC_CHANNEL = ADC_CHANNEL_5; /**< GPIO6 on ESP32-S3 */

    // Voltage Divider: R1=100k (top), R2=100k (bottom)
    // V_ADC = VBAT * 100k / (100k + 100k) = VBAT * 0.5
    // VBAT = V_ADC * 2.0
    static constexpr float VOLTAGE_DIVIDER_RATIO = 2.0f; /**< Voltage divider ratio for battery monitoring */

    // Battery Thresholds (in millivolts)
    static constexpr uint32_t LOW_BATTERY_MV = 3000;     /**< 3.0V cutoff - enter deep sleep below this */
    static constexpr uint32_t WARNING_BATTERY_MV = 3200; /**< 3.2V warning threshold - alert before shutdown */
    static constexpr uint32_t FULL_BATTERY_MV = 4200;    /**< 4.2V full charge */

    // Timing Configuration
    static constexpr uint32_t FAN_ON_TIME_MS = 30000; /**< Fan runs for 30 seconds per press */
    static constexpr uint32_t ADC_INTERVAL_MS = 5000; /**< Check battery every 5 seconds */
    static constexpr uint32_t DEBOUNCE_TIME_MS = 50;  /**< Button debounce time */

    // Button Timing
    static constexpr uint32_t LONG_PRESS_TIME_MS = 1000; /**< 1 second hold = long press (stop fan) */

    // Timer Stacking
    static constexpr uint32_t MAX_PRESS_COUNT = 5;      /**< Maximum button presses to stack */
    static constexpr uint32_t MAX_FAN_TIME_MS = 150000; /**< 150 seconds max (5 × 30s) */

    // LED Timing
    static constexpr uint32_t LED_BLINK_ON_MS = 150;  /**< LED on duration per blink */
    static constexpr uint32_t LED_BLINK_OFF_MS = 150; /**< LED off duration between blinks */

    // FreeRTOS Task Configuration
    static constexpr uint32_t ADC_TASK_STACK_SIZE = 4096;
    static constexpr UBaseType_t ADC_TASK_PRIORITY = 2;

    static constexpr uint32_t BUTTON_TASK_STACK_SIZE = 4096;
    static constexpr UBaseType_t BUTTON_TASK_PRIORITY = 3;

    static constexpr uint32_t FAN_TASK_STACK_SIZE = 4096;
    static constexpr UBaseType_t FAN_TASK_PRIORITY = 2;

    // Queue Configuration
    static constexpr uint32_t BUTTON_QUEUE_SIZE = 5;
    static constexpr uint32_t FAN_QUEUE_SIZE = 5;

    // Fan Command Types
    enum class FanCommand
    {
        On,      /**< Start fan (30s) - first press */
        AddTime, /**< Add 30s to running fan - subsequent presses */
        Off,     /**< Stop fan (auto-off or long press) */
        Shutdown /**< Low battery shutdown */
    };

    /**
     * @brief Validate all configuration parameters
     * @return true if all parameters are valid, false otherwise
     */
    static bool validate();
};

#endif // CONFIG_HPP