#ifndef FAN_CONTROLLER_HPP
#define FAN_CONTROLLER_HPP

#include "config.hpp"
#include <cstdint>

// Forward declaration for state machine
enum class FanState : uint8_t {
    Off,      // Fan is off
    On,       // Fan is on (running)
    AddingTime, // Fan is on but adding time
    Shutdown  // System shutdown state
};

namespace fan_controller {

/**
 * @brief Initialize the fan controller.
 * @details Configures GPIO5 as output for MOSFET gate control.
 * @return ESP_OK on success, other ESP error code on failure
 */
esp_err_t init();

/**
 * @brief Start the fan controller task.
 * @details The task handles fan ON/OFF commands and auto-off timer.
 */
void start();

/**
 * @brief Send a command to the fan controller.
 * @param cmd The command to send to the fan controller
 * @return ESP_OK on success, other ESP error code on failure
 */
esp_err_t send_cmd(Config::FanCommand cmd);

/**
 * @brief Check if the fan is currently on.
 * @return true if the fan is on, false otherwise
 */
bool is_on();

/**
 * @brief Get current press count (1-5, or 0 if fan is off).
 * @return Number of presses (1-5, or 0 if fan is off)
 */
uint8_t get_press_count();

/**
 * @brief Get current fan state.
 * @return Current fan state
 */
FanState get_state();

/**
 * @brief Turn fan on directly (bypasses queue).
 * @details Immediately turns on the fan without queueing a command.
 * @return ESP_OK on success, other ESP error code on failure
 */
esp_err_t on();

/**
 * @brief Turn fan off directly (bypasses queue).
 * @details Immediately turns off the fan without queueing a command.
 * @return ESP_OK on success, other ESP error code on failure
 */
esp_err_t off();

} // namespace fan_controller

#endif // FAN_CONTROLLER_HPP
