#ifndef LED_CONTROLLER_HPP
#define LED_CONTROLLER_HPP

#include <cstdint>
#include "esp_err.h"

/**
 * @brief LED controller namespace for ESP32-S3 Fan Controller
 *
 * This namespace handles status LED indication for the fan controller,
 * including blinking patterns for different system states.
 */
namespace led_controller {

/**
 * @brief Initialize the LED controller.
 * @details Configures the specified GPIO pin as output for status LED.
 * @return ESP_OK on success, other ESP error code on failure
 */
esp_err_t init();

/**
 * @brief Blink the LED N times (blocking).
 * @details Each blink is LED_BLINK_ON_MS on, LED_BLINK_OFF_MS off.
 * @param count Number of times to blink (1-5 typical)
 */
void blink(uint8_t count);

/**
 * @brief Blink the LED with low battery warning pattern (5 quick blinks).
 */
void blink_low_battery();

/**
 * @brief Turn LED on.
 */
void on();

/**
 * @brief Turn LED off.
 */
void off();

} // namespace led_controller

#endif // LED_CONTROLLER_HPP
