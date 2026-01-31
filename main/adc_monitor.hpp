#ifndef ADC_MONITOR_HPP
#define ADC_MONITOR_HPP

#include <cstdint>
#include "esp_err.h"

/**
 * @brief ADC monitoring namespace for ESP32-S3 Fan Controller
 *
 * This namespace handles battery voltage monitoring using the ESP32's ADC
 * capabilities to track battery level and trigger low battery shutdown.
 */
namespace adc_monitor {

/**
 * @brief Initialize the ADC monitor.
 * @details Configures the ADC peripheral for battery voltage monitoring.
 * @return ESP_OK on success, other ESP error code on failure
 */
esp_err_t init();

/**
 * @brief Start the ADC monitor task.
 * @details The task reads battery voltage every ADC_INTERVAL_MS and triggers
 * shutdown if voltage drops below LOW_BATTERY_MV.
 */
void start();

/**
 * @brief Get the current battery voltage in millivolts.
 * @details Returns the last measured battery voltage.
 * @return Battery voltage in millivolts
 */
uint32_t get_voltage_mv();

/**
 * @brief Read and return the current battery voltage in millivolts.
 * @details This performs an immediate ADC reading.
 * @return Battery voltage in millivolts
 */
uint32_t read_voltage_mv();

} // namespace adc_monitor

#endif // ADC_MONITOR_HPP
