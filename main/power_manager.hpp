#ifndef POWER_MANAGER_HPP
#define POWER_MANAGER_HPP

#include "esp_sleep.h"
#include "esp_err.h"

/**
 * @brief Power management namespace for ESP32-S3 Fan Controller
 *
 * This namespace handles deep sleep functionality, wake-up cause detection,
 * and GPIO management during sleep states to minimize power consumption.
 */
namespace power_manager {

/**
 * @brief Initialize the power manager.
 * @details Configures the specified GPIO pin as a deep sleep wake source.
 * @return ESP_OK on success, other ESP error code on failure
 */
esp_err_t init();

/**
 * @brief Enter deep sleep mode.
 * @details Wakes up on button press (GPIO4 LOW).
 * Call this on low battery to minimize power consumption.
 * @note This function does not return.
 */
[[noreturn]] void enter_deep_sleep();

/**
 * @brief Check the wake-up cause.
 * @return The reason why ESP32 woke up from sleep
 */
esp_sleep_wakeup_cause_t get_wakeup_cause();

/**
 * @brief Check if woke up from deep sleep (vs power-on reset).
 * @return true if woke up from deep sleep, false if power-on reset
 */
bool woke_from_deep_sleep();

} // namespace power_manager

#endif // POWER_MANAGER_HPP
