# ESP32-S3 Battery-Powered Fan Controller

## Project Overview

A simple battery-powered fan controller that:
- Turns on a fan for 30 seconds when button is pressed
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

### Bill of Materials

| Ref | Component | Value | Notes |
|-----|-----------|-------|-------|
| U1 | ESP32-S3 DevKit | - | Any ESP32-S3 dev board with onboard LDO |
| Q1 | N-MOSFET | Si2302CDS | SOT-23, or equivalent (2N7002, AO3400) |
| D1 | Schottky Diode | SS14 | Flyback protection for motor |
| R1 | Resistor | 100k | Voltage divider top |
| R2 | Resistor | 100k | Voltage divider bottom |
| R3 | Resistor | 10k | MOSFET gate pull-down |
| SW1 | Slide/Toggle Switch | SPST | Main power switch |
| SW2 | Tactile Button | 6x6mm | Fan trigger button |
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

// Timing
FAN_ON_TIME_MS   = 30000   // 30 seconds
ADC_INTERVAL_MS  = 5000    // Check battery every 5 seconds
DEBOUNCE_TIME_MS = 50      // Button debounce

// Battery
LOW_BATTERY_MV   = 3000    // 3.0V cutoff
FULL_BATTERY_MV  = 4200    // 4.2V full

// Voltage divider ratio (100k/100k)
VOLTAGE_DIVIDER_RATIO = 2.0
```

### Source Files

```
main/
├── CMakeLists.txt      # Build configuration
├── config.hpp          # Pin definitions, constants
├── main.cpp            # Entry point, initialization
├── power_manager.hpp   # Deep sleep interface
├── power_manager.cpp   # Deep sleep implementation
├── adc_monitor.hpp     # Battery monitoring interface
├── adc_monitor.cpp     # ADC reading, low battery detection
├── button_handler.hpp  # Button interface
├── button_handler.cpp  # Button interrupt, debounce
├── fan_controller.hpp  # Fan control interface
└── fan_controller.cpp  # MOSFET control, auto-off timer
```

### FreeRTOS Tasks

| Task | Stack | Priority | Function |
|------|-------|----------|----------|
| adc_monitor | 2048 | 2 | Read battery voltage every 5s |
| button_handler | 2048 | 3 | Handle button interrupts |
| fan_controller | 2048 | 2 | Control fan, auto-off timer |

### Behavior

1. **Power On** (SW1): ESP32 boots, initializes all subsystems
2. **Button Press** (SW2): Fan turns on for 30 seconds
3. **Auto-Off**: Fan turns off after 30 seconds
4. **Low Battery**: When VBAT < 3.0V, fan off, enter deep sleep
5. **Wake Up**: Button press wakes ESP32 from deep sleep

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
- GPIO5, GPIO6 isolated during sleep to minimize current
- Wake-up cause logged on boot

## Testing Checklist

- [ ] Power on with SW1, verify 3.3V on ESP32
- [ ] Measure GPIO6 voltage (should be VBAT × 0.6)
- [ ] Press SW2, verify fan turns on
- [ ] Wait 30s, verify fan auto-off
- [ ] Reduce battery to <3V, verify deep sleep
- [ ] Press button in deep sleep, verify wake-up

## Hardware Files

- `hardware/schematic.txt` - Detailed ASCII schematic with assembly notes
