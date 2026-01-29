#include "adc_monitor.hpp"
#include "config.hpp"
#include "power_manager.hpp"
#include "fan_controller.hpp"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <atomic>

namespace {
    const char* TAG = "adc_monitor";

    adc_oneshot_unit_handle_t adc_handle = nullptr;
    adc_cali_handle_t cali_handle = nullptr;
    bool calibration_enabled = false;
    std::atomic<uint32_t> last_voltage_mv{0};

    bool init_calibration()
    {
        esp_err_t ret;
        bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = ADC_UNIT_1,
            .chan = BATTERY_ADC_CHANNEL,
            .atten = ADC_ATTEN,
            .bitwidth = ADC_WIDTH,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &cali_handle);
        if (ret == ESP_OK) {
            calibrated = true;
            ESP_LOGI(TAG, "ADC calibration: curve fitting");
        }
#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = ADC_UNIT_1,
            .atten = ADC_ATTEN,
            .bitwidth = ADC_WIDTH,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &cali_handle);
        if (ret == ESP_OK) {
            calibrated = true;
            ESP_LOGI(TAG, "ADC calibration: line fitting");
        }
#endif

        if (!calibrated) {
            ESP_LOGW(TAG, "ADC calibration not available, using raw values");
        }

        return calibrated;
    }

    void monitor_task(void* pvParameters)
    {
        ESP_LOGI(TAG, "ADC monitor task started");

        uint32_t voltage = adc_monitor::read_voltage_mv();
        ESP_LOGI(TAG, "Initial battery voltage: %lu mV", static_cast<unsigned long>(voltage));

        while (true) {
            vTaskDelay(pdMS_TO_TICKS(ADC_INTERVAL_MS));

            voltage = adc_monitor::read_voltage_mv();
            ESP_LOGI(TAG, "Battery voltage: %lu mV", static_cast<unsigned long>(voltage));

            if (voltage < LOW_BATTERY_MV && voltage > 0) {
                ESP_LOGW(TAG, "Low battery detected! (%lu mV < %lu mV)",
                         static_cast<unsigned long>(voltage),
                         static_cast<unsigned long>(LOW_BATTERY_MV));

                // Turn off fan before sleeping
                fan_controller::send_cmd(FanCommand::Shutdown);
                vTaskDelay(pdMS_TO_TICKS(100));

                // Enter deep sleep to conserve power
                power_manager::enter_deep_sleep();
            }
        }
    }
}

namespace adc_monitor {

void init()
{
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
        .clk_src = ADC_RTC_CLK_SRC_DEFAULT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN,
        .bitwidth = ADC_WIDTH,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, BATTERY_ADC_CHANNEL, &config));

    calibration_enabled = init_calibration();

    ESP_LOGI(TAG, "ADC monitor initialized on GPIO%d (ADC1_CH%d)",
             BATTERY_ADC_PIN, BATTERY_ADC_CHANNEL);
}

void start()
{
    xTaskCreate(
        monitor_task,
        "adc_monitor",
        ADC_TASK_STACK_SIZE,
        nullptr,
        ADC_TASK_PRIORITY,
        nullptr
    );
}

uint32_t read_voltage_mv()
{
    int raw_value = 0;
    int voltage_mv = 0;

    esp_err_t ret = adc_oneshot_read(adc_handle, BATTERY_ADC_CHANNEL, &raw_value);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ADC read failed: %s", esp_err_to_name(ret));
        return last_voltage_mv.load();
    }

    if (calibration_enabled && cali_handle != nullptr) {
        ret = adc_cali_raw_to_voltage(cali_handle, raw_value, &voltage_mv);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "ADC calibration conversion failed, using raw estimate");
            voltage_mv = (raw_value * 2500) / 4095;
        }
    } else {
        voltage_mv = (raw_value * 2500) / 4095;
    }

    uint32_t battery_mv = static_cast<uint32_t>(voltage_mv * VOLTAGE_DIVIDER_RATIO);
    last_voltage_mv.store(battery_mv);

    return battery_mv;
}

uint32_t get_voltage_mv()
{
    return last_voltage_mv.load();
}

} // namespace adc_monitor
