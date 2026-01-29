#ifndef POWER_MANAGER_HPP
#define POWER_MANAGER_HPP

#include "esp_sleep.h"

namespace power_manager {

/**
 * Initialize the power manager.
 * Configures GPIO4 as deep sleep wake source.
 */
void init();

/**
 * Enter deep sleep mode.
 * Wakes up on button press (GPIO4 LOW).
 * Call this on low battery to minimize power consumption.
 */
[[noreturn]] void enter_deep_sleep();

/**
 * Check the wake-up cause.
 * Returns the reason why ESP32 woke up from sleep.
 */
esp_sleep_wakeup_cause_t get_wakeup_cause();

/**
 * Check if woke up from deep sleep (vs power-on reset).
 */
bool woke_from_deep_sleep();

} // namespace power_manager

#endif // POWER_MANAGER_HPP
