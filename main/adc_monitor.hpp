#ifndef ADC_MONITOR_HPP
#define ADC_MONITOR_HPP

#include <cstdint>

namespace adc_monitor {

/**
 * Initialize the ADC monitor.
 * Configures ADC1_CH5 (GPIO6) for battery voltage monitoring.
 */
void init();

/**
 * Start the ADC monitor task.
 * The task reads battery voltage every ADC_INTERVAL_MS and triggers
 * shutdown if voltage drops below LOW_BATTERY_MV.
 */
void start();

/**
 * Get the current battery voltage in millivolts.
 * Returns the last measured battery voltage.
 */
uint32_t get_voltage_mv();

/**
 * Read and return the current battery voltage in millivolts.
 * This performs an immediate ADC reading.
 */
uint32_t read_voltage_mv();

} // namespace adc_monitor

#endif // ADC_MONITOR_HPP
