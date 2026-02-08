# Codex Project: ESP32-S3 Battery-Powered Fan Controller

## Overview
- Battery-powered fan controller firmware for ESP32-S3.
- Short press starts fan for 30s; each additional press adds 30s up to 5 presses (150s max).
- Long press (>1s) stops the fan immediately.
- LED indicates stacked presses; low-battery warning and cutoff with deep sleep.

## Hardware Pins
| GPIO | Function | Notes |
|---|---|---|
| GPIO4 | Button | Input, internal pull-up, active LOW, deep sleep wake |
| GPIO5 | Fan MOSFET gate | Output, 10k pull-down to GND |
| GPIO6 | Battery ADC | Input, ADC1_CH5, divider input |
| GPIO7 | Status LED | Output, 220 ohm series resistor |

## Electrical Constraints
- Voltage divider: 100k/100k, `V_ADC = V_BAT * 0.5`, `V_BAT = V_ADC * 2.0`.
- Battery thresholds:
  - Warning: 3.2V (LED blinks 5x).
  - Cutoff: 3.0V (deep sleep to protect cell).
- ADC config: 12-bit, 12 dB attenuation (0-2.5V range).

## Code Layout
- `main/config.*` - central configuration and validation.
- `main/main.cpp` - entry point, boot, module init.
- `main/power_manager.*` - deep sleep handling.
- `main/adc_monitor.*` - battery monitoring.
- `main/button_handler.*` - button interrupts, debounce, long press.
- `main/fan_controller.*` - fan control, timer stacking, state machine.
- `main/led_controller.*` - LED patterns.

## Build / Flash / Monitor
```bash
# Use settings from .vscode/settings.json (ESP-IDF v5.5.1, tools path, target).
# VS Code ESP-IDF extension uses this Python venv:
IDF_PATH=~/esp/v5.5.1/esp-idf \
IDF_TOOLS_PATH=~/.espressif \
IDF_TARGET=esp32s3 \
IDF_PYTHON_ENV_PATH=~/.espressif/python_env/idf5.5_py3.12_env \
~/.espressif/python_env/idf5.5_py3.12_env/bin/python \
  ~/esp/v5.5.1/esp-idf/tools/idf.py build
```

## Runtime Behavior
- Short press: fan on 30s, LED blinks 1x.
- Additional short presses while running: add 30s, LED blinks N (up to 5).
- Long press (>1s): fan stops immediately, timer reset.
- Low battery (<3.2V): LED blinks 5x warning.
- Critical battery (<3.0V): fan off, deep sleep.
- Wake from deep sleep: button press (GPIO4, EXT0).

## Testing Checklist
- Boot logs show configuration validation pass.
- Short press starts fan + 1x blink.
- Multiple presses stack time; 6th press ignored.
- Long press stops fan immediately.
- Auto-off after timer expiry.
- Low battery warning at ~3.1V (5x blink).
- Deep sleep below 3.0V; button wakes.

## Troubleshooting Notes
- Ensure target is `esp32s3` and ESP-IDF v5.5.1 is active (matches `sdkconfig`).
- Deep sleep wake source: EXT0 on GPIO4 (LOW).
- Verify ADC divider wiring (GPIO6) if battery readings are off.
- If `No module named 'click'` appears, the configured Python env lacks ESP-IDF deps.
