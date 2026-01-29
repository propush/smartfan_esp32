#ifndef BUTTON_HANDLER_HPP
#define BUTTON_HANDLER_HPP

namespace button_handler {

/**
 * Initialize the button handler.
 * Configures GPIO4 as input with interrupt on falling edge (button press).
 */
void init();

/**
 * Start the button handler task.
 * The task debounces button presses and sends events to the fan controller.
 */
void start();

/**
 * Check if the button is currently pressed.
 * Returns true if GPIO4 is LOW (button active).
 */
bool is_pressed();

} // namespace button_handler

#endif // BUTTON_HANDLER_HPP
