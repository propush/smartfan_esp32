# ESP32-S3 Battery-Powered Fan Controller

## Project Overview

A simple battery-powered fan controller that:
- Turns on a fan for 30 seconds when button is pressed
- Supports timer stacking: each press adds 30s (up to 5 presses = 150s max)
- Long press (>1s) stops fan immediately
- LED blinks N times to indicate N stacked presses
- Monitors battery voltage via ADC
- Enters deep sleep when battery is low (<3.0V) to protect the cell
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
| 3.0V | 1.50V | Cutoff → Deep Sleep |

## Software Configuration

### Constants (config.hpp)

```cpp
// Pins
GPIO4  - BUTTON_PIN
GPIO5  - FAN_GATE_PIN
GPIO6  - BATTERY_ADC_PIN
GPIO7  - LED_PIN

// Timing
FAN_ON_TIME_MS     = 30000   // 30 seconds per press
ADC_INTERVAL_MS    = 5000    // Check battery every 5 seconds
DEBOUNCE_TIME_MS   = 50      // Button debounce
LONG_PRESS_TIME_MS = 1000    // 1 second = long press (stop fan)

// Timer Stacking
MAX_PRESS_COUNT    = 5       // Max 5 presses
MAX_FAN_TIME_MS    = 150000  // 150 seconds max (5 × 30s)

// LED Timing
LED_BLINK_ON_MS    = 150     // LED on duration per blink
LED_BLINK_OFF_MS   = 150     // LED off duration between blinks

// Battery
LOW_BATTERY_MV     = 3000    // 3.0V cutoff
FULL_BATTERY_MV    = 4200    // 4.2V full

// Voltage divider ratio (100k/100k)
VOLTAGE_DIVIDER_RATIO = 2.0
```

### Source Files

```
main/
├── CMakeLists.txt       # Build configuration
├── config.hpp           # Pin definitions, constants
├── main.cpp             # Entry point, initialization
├── power_manager.hpp    # Deep sleep interface
├── power_manager.cpp    # Deep sleep implementation
├── adc_monitor.hpp      # Battery monitoring interface
├── adc_monitor.cpp      # ADC reading, low battery detection
├── button_handler.hpp   # Button interface
├── button_handler.cpp   # Button interrupt, debounce, long press
├── fan_controller.hpp   # Fan control interface
├── fan_controller.cpp   # MOSFET control, timer stacking
├── led_controller.hpp   # LED control interface
└── led_controller.cpp   # LED blink patterns
```

### FreeRTOS Tasks

| Task | Stack | Priority | Function |
|------|-------|----------|----------|
| adc_monitor | 4096 | 2 | Read battery voltage every 5s |
| button_handler | 2048 | 3 | Handle button interrupts, detect long press |
| fan_controller | 4096 | 2 | Control fan, timer stacking, LED feedback |

### Behavior

1. **Power On** (SW1): ESP32 boots, initializes all subsystems
2. **Short Press** (SW2):
   - If fan off: Start fan for 30s, LED blinks 1×
   - If fan on: Add 30s (up to 5 presses max), LED blinks N× for N presses
3. **Long Press** (hold >1s): Stop fan immediately, reset timer
4. **Auto-Off**: Fan turns off when timer expires
5. **Low Battery**: When VBAT < 3.0V, fan off, enter deep sleep
6. **Wake Up**: Button press wakes ESP32 from deep sleep

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

- [ ] Power on with SW1, verify 3.3V on ESP32
- [ ] Measure GPIO6 voltage (should be VBAT × 0.5)
- [ ] Short press SW2, verify fan on + LED blinks 1×
- [ ] Press again while running, verify LED blinks 2×
- [ ] Press 5 times total, verify LED blinks 5× (max)
- [ ] Press 6th time, verify no response (max reached)
- [ ] Long press (>1s), verify fan stops immediately
- [ ] Wait for timer to expire, verify auto-off
- [ ] Reduce battery to <3V, verify deep sleep
- [ ] Press button in deep sleep, verify wake-up

## Hardware Files

- `hardware/schematic.txt` - Detailed ASCII schematic with assembly notes
