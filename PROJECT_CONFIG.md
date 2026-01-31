# ESP32-S3 Battery-Powered Fan Controller

## Project Overview

A simple battery-powered fan controller that:
- Turns on a fan for 30 seconds when button is pressed
- Supports timer stacking: each press adds 30s (up to 5 presses = 150s max)
- Long press (>1s) stops fan immediately
- LED blinks N times to indicate N stacked presses
- Monitors battery voltage via ADC
- LED blinks 5× when battery is low (<3.2V) as warning
- Enters deep sleep when battery is critical (<3.0V) to protect the cell
- Wakes from deep sleep on button press

## Hardware Configuration

### Pin Assignments

| GPIO | Function | Direction | Notes |
|------|----------|-----------|-------|
| GPIO4 | Button | Input | Internal pull-up, active LOW, deep sleep wake source |
| GPIO5 | Fan MOSFET Gate | Output | 10k pull-down resistor to GND |
| GPIO6 | Battery ADC | Input | ADC1_CH5, voltage divider input |
| GPIO7 | Status LED | Output | 220 ohm current limiting resistor |

### Bill of Materials

| Ref | Component | Value | Notes |
|-----|-----------|-------|-------|
| U1 | ESP32-S3 DevKit | - | Any ESP32-S3 dev board with onboard LDO |
| Q1 | N-MOSFET | Si2302CDS | SOT-23, or equivalent (2N7002, AO3400) |
| D1 | Schottky Diode | SS14 | Flyback protection for motor |
| R1 | Resistor | 100k | Voltage divider top |
| R2 | Resistor | 100k | Voltage divider bottom |
| R3 | Resistor | 10k | MOSFET gate pull-down |
| R4 | Resistor | 220 | LED current limiter |
| SW1 | Slide/Toggle Switch | SPST | Main power switch |
| SW2 | Tactile Button | 6x6mm | Fan trigger button |
| LED1 | LED | 3mm/5mm | Status indicator (any color) |
| BAT1 | Li-Ion Battery | 3.7V | 18650 or similar |
| FAN1 | DC Fan | 5V | Runs at reduced speed on 3.7V |

### Circuit Description

```
Li-Ion Battery (3.0-4.2V)
        |
   [SW1 - Power Switch]
        |
      VBAT
        |
   +----+----+--------------------+
   |    |    |                    |
   |   R1   VIN                  FAN+
   |  100k   |                    |
   |    |   ESP32              FAN-
   |    +---GPIO6                 |
   |    |                    +----+----+
   |   R2                    |    |    |
   |  100k                  D1   DRAIN |
   |    |                    |    |    |
   GND  GND              VBAT    Q1    |
                              (MOSFET) |
                                 |     |
                            GATE-+     |
                                 |   SOURCE
                                R3     |
                               10k    GND
                                 |
                           GPIO5-+

Button: GPIO4 ----[SW2]---- GND
        (internal pull-up enabled)

LED: GPIO7 ---[R4 220]---[LED1]--- GND
```

### Voltage Divider Calculations

```
V_ADC = VBAT × R2 / (R1 + R2)
V_ADC = VBAT × 100k / 200k
V_ADC = VBAT × 0.5

VBAT = V_ADC × 2.0
```

| Battery | ADC Voltage | Status |
|---------|-------------|--------|
| 4.2V | 2.10V | Full |
| 3.7V | 1.85V | Nominal |
| 3.2V | 1.60V | Warning → LED blinks 5× |
| 3.0V | 1.50V | Cutoff → Deep Sleep |

## Software Configuration

### Configuration Class (config.hpp, config.cpp)

The project uses a centralized `Config` class to manage all system constants with compile-time configuration and runtime validation.

#### Pin Assignments (Config Class)
```cpp
static constexpr gpio_num_t BUTTON_PIN = GPIO_NUM_4;      // Button input
static constexpr gpio_num_t FAN_GATE_PIN = GPIO_NUM_5;    // Fan MOSFET gate
static constexpr gpio_num_t BATTERY_ADC_PIN = GPIO_NUM_6; // Battery ADC (ADC1_CH5)
static constexpr gpio_num_t LED_PIN = GPIO_NUM_7;         // Status LED
```

#### Timing Constants
```cpp
FAN_ON_TIME_MS     = 30000   // 30 seconds per press
ADC_INTERVAL_MS    = 5000    // Check battery every 5 seconds
DEBOUNCE_TIME_MS   = 50      // Button debounce
LONG_PRESS_TIME_MS = 1000    // 1 second = long press (stop fan)
```

#### Timer Stacking
```cpp
MAX_PRESS_COUNT    = 5       // Max 5 presses
MAX_FAN_TIME_MS    = 150000  // 150 seconds max (5 × 30s)
```

#### LED Timing
```cpp
LED_BLINK_ON_MS    = 150     // LED on duration per blink
LED_BLINK_OFF_MS   = 150     // LED off duration between blinks
```

#### Battery Thresholds
```cpp
LOW_BATTERY_MV     = 3000    // 3.0V cutoff → deep sleep
WARNING_BATTERY_MV = 3200    // 3.2V warning → LED blinks 5×
FULL_BATTERY_MV    = 4200    // 4.2V full
```

#### ADC & Voltage Divider
```cpp
ADC_ATTEN = ADC_ATTEN_DB_12;           // 0-2.5V range
ADC_WIDTH = ADC_BITWIDTH_12;           // 12-bit resolution
VOLTAGE_DIVIDER_RATIO = 2.0f;          // 100k/100k divider
```

#### Configuration Validation

The `Config::validate()` method performs startup checks:
- Battery thresholds are properly ordered (LOW < WARNING < FULL)
- Timing values are non-zero and consistent
- GPIO pins are unique (no conflicts)
- MAX_FAN_TIME_MS ≥ FAN_ON_TIME_MS

Validation runs automatically on boot and logs any configuration errors.

### Source Files

```
main/
├── CMakeLists.txt       # Build configuration
├── config.hpp           # Configuration class, pin definitions, constants
├── config.cpp           # Configuration validation implementation
├── main.cpp             # Entry point, initialization, startup validation
├── power_manager.hpp    # Deep sleep interface
├── power_manager.cpp    # Deep sleep implementation
├── adc_monitor.hpp      # Battery monitoring interface
├── adc_monitor.cpp      # ADC reading, low battery detection
├── button_handler.hpp   # Button interface
├── button_handler.cpp   # Button interrupt, debounce, long press
├── fan_controller.hpp   # Fan control interface, state machine
├── fan_controller.cpp   # MOSFET control, timer stacking, state management
├── led_controller.hpp   # LED control interface
└── led_controller.cpp   # LED blink patterns
```

### Fan Controller State Machine

The fan controller implements a state machine pattern for robust state management:

```cpp
enum class FanState : uint8_t {
    Off,        // Fan is off
    On,         // Fan is on (running)
    AddingTime, // Fan is on but adding time
    Shutdown    // System shutdown state
};
```

**State Transitions:**
- `Off` → `On`: First button press starts fan
- `On` → `AddingTime`: Subsequent press while running (up to 5 max)
- `On`/`AddingTime` → `Off`: Long press or timer expires
- Any state → `Shutdown`: Low battery condition

**Fan Commands:**
```cpp
enum class FanCommand {
    On,      // Start fan (30s) - first press
    AddTime, // Add 30s to running fan
    Off,     // Stop fan (auto-off or long press)
    Shutdown // Low battery shutdown
};
```

### Architecture & Code Quality

**Key improvements implemented:**
- **Error Handling**: All initialization and operations return `esp_err_t` codes
- **Doxygen Documentation**: Comprehensive function and class documentation
- **Configuration Validation**: Runtime checks for parameter consistency
- **Modular Design**: Clear separation of concerns with defined interfaces
- **State Management**: Explicit state machine for fan controller logic

### FreeRTOS Tasks

| Task | Stack | Priority | Function |
|------|-------|----------|----------|
| adc_monitor | 4096 | 2 | Read battery voltage every 5s |
| button_handler | 4096 | 3 | Handle button interrupts, detect long press |
| fan_controller | 4096 | 2 | Control fan, timer stacking, LED feedback |

### Behavior

1. **Power On** (SW1):
   - ESP32 boots, runs configuration validation
   - Initializes all subsystems (power manager, fan, LED, ADC, button)
   - Checks initial battery voltage
   - Enters deep sleep if battery critically low on startup
2. **Short Press** (SW2):
   - If fan off: Start fan for 30s, LED blinks 1×
   - If fan on: Add 30s (up to 5 presses max), LED blinks N× for N presses
3. **Long Press** (hold >1s): Stop fan immediately, reset timer
4. **Auto-Off**: Fan turns off when timer expires
5. **Low Battery Warning**: When VBAT < 3.2V, LED blinks 5× as warning
6. **Low Battery Shutdown**: When VBAT < 3.0V, fan off, enter deep sleep
7. **Wake Up**: Button press wakes ESP32 from deep sleep

### Button Behavior Summary

| Action | Fan State | Result |
|--------|-----------|--------|
| Short press | Off | Fan on 30s, LED blinks 1× |
| Short press | On (1-4 presses) | Add 30s, LED blinks N× |
| Short press | On (5 presses) | Ignored (max reached) |
| Long press (>1s) | Any | Fan off, timer reset |

## Build Instructions

### Prerequisites

- ESP-IDF v5.x installed
- ESP32-S3 target configured

### Build Commands

```bash
# Set up ESP-IDF environment
source ~/esp/esp-idf/export.sh

# Configure for ESP32-S3
idf.py set-target esp32s3

# Build
idf.py build

# Flash and monitor
idf.py flash monitor
```

### CMakeLists.txt Dependencies

```cmake
REQUIRES
    driver
    esp_adc
    nvs_flash
    esp_timer
```

## Deep Sleep Details

- Wake source: EXT0 on GPIO4 (LOW level)
- Current in deep sleep: ~10µA
- GPIO5, GPIO6, GPIO7 isolated during sleep to minimize current
- Wake-up cause logged on boot

## Testing Checklist

### Basic Functionality
- [ ] Power on with SW1, verify 3.3V on ESP32
- [ ] Check serial output for "Configuration validation passed"
- [ ] Verify initial battery voltage reading logged
- [ ] Measure GPIO6 voltage (should be VBAT × 0.5)

### Fan Operation
- [ ] Short press SW2, verify fan on + LED blinks 1×
- [ ] Press again while running, verify LED blinks 2×
- [ ] Press 5 times total, verify LED blinks 5× (max)
- [ ] Press 6th time, verify no response (max reached)
- [ ] Long press (>1s), verify fan stops immediately
- [ ] Wait for timer to expire, verify auto-off

### Battery Management
- [ ] Reduce battery to ~3.1V, verify LED blinks 5× (low battery warning)
- [ ] Reduce battery to <3V, verify deep sleep
- [ ] Press button in deep sleep, verify wake-up
- [ ] Power on with battery <3V, verify immediate deep sleep

### Error Handling
- [ ] Check all module initialization returns ESP_OK
- [ ] Verify proper error logging if initialization fails
- [ ] Confirm system continues operation after non-critical errors

## Recent Implementation Improvements

### Version 2.0 - Enhanced Code Quality & Architecture

**Configuration Management:**
- Centralized configuration class (`Config`) with all system constants
- Runtime configuration validation with detailed error reporting
- Better organization of hardware, timing, and threshold parameters

**Error Handling:**
- All modules return `esp_err_t` for proper error propagation
- Comprehensive error checking throughout initialization and runtime
- Detailed logging for debugging and monitoring

**State Machine:**
- Fan controller uses explicit state machine pattern
- Clear state transitions with `FanState` enum
- Command-based control with `FanCommand` enum

**Code Quality:**
- Doxygen-style documentation for all functions and classes
- Consistent naming conventions and formatting
- Improved code maintainability and readability

**Testing & Debugging:**
- Enhanced logging with granular messages
- Better error reporting for diagnostics
- More testable components with proper interfaces

See `IMPLEMENTATION_SUMMARY.md` for detailed change documentation.

## Hardware Files

- `hardware/schematic.txt` - Detailed ASCII schematic with assembly notes
- `IMPLEMENTATION_SUMMARY.md` - Software implementation change log
