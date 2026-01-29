#ifndef FAN_CONTROLLER_HPP
#define FAN_CONTROLLER_HPP

#include "config.hpp"
#include <cstdint>

namespace fan_controller {

/**
 * Initialize the fan controller.
 * Configures GPIO5 as output for MOSFET gate control.
 */
void init();

/**
 * Start the fan controller task.
 * The task handles fan ON/OFF commands and auto-off timer.
 */
void start();

/**
 * Send a command to the fan controller.
 */
void send_cmd(FanCommand cmd);

/**
 * Check if the fan is currently on.
 */
bool is_on();

/**
 * Get current press count (1-5, or 0 if fan is off).
 */
uint8_t get_press_count();

/**
 * Turn fan on directly (bypasses queue).
 */
void on();

/**
 * Turn fan off directly (bypasses queue).
 */
void off();

} // namespace fan_controller

#endif // FAN_CONTROLLER_HPP
