#ifndef LED_CONTROLLER_HPP
#define LED_CONTROLLER_HPP

#include <cstdint>

namespace led_controller {

/**
 * Initialize the LED controller.
 * Configures GPIO7 as output for status LED.
 */
void init();

/**
 * Blink the LED N times (blocking).
 * Each blink is LED_BLINK_ON_MS on, LED_BLINK_OFF_MS off.
 * @param count Number of times to blink (1-5 typical)
 */
void blink(uint8_t count);

/**
 * Turn LED on.
 */
void on();

/**
 * Turn LED off.
 */
void off();

} // namespace led_controller

#endif // LED_CONTROLLER_HPP
