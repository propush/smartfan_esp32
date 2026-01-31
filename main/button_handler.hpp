#ifndef BUTTON_HANDLER_HPP
#define BUTTON_HANDLER_HPP

#include "esp_err.h"

/**
 * @brief Button handler namespace for ESP32-S3 Fan Controller
 *
 * This namespace handles button press detection and debouncing using
 * GPIO interrupts to control fan operation.
 */
namespace button_handler {

/**
 * @brief Initialize the button handler.
 * @details Configures the specified GPIO pin as input with interrupt support.
 * @return ESP_OK on success, other ESP error code on failure
 */
esp_err_t init();

/**
 * @brief Start the button handler task.
 * @details The task handles button press detection, debouncing, and
 * sends events to the fan controller.
 */
void start();

/**
 * @brief Check if the button is currently pressed.
 * @return true if GPIO is LOW (button active), false otherwise
 */
bool is_pressed();

} // namespace button_handler

#endif // BUTTON_HANDLER_HPP
