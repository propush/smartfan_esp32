#include "fan_controller.hpp"
#include "config.hpp"
#include "led_controller.hpp"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <atomic>

namespace {
    const char* TAG = "fan_controller";

    QueueHandle_t cmd_queue = nullptr;
    TaskHandle_t task_handle = nullptr;
    std::atomic<bool> fan_is_on{false};
    std::atomic<uint8_t> press_count{0};
    std::atomic<FanState> current_state{FanState::Off};
    int64_t fan_off_time_us = 0;  // Absolute time when fan should turn off

    void handle_on_command()
    {
        // First press: start fan with 30s timer
        press_count.store(1);
        fan_off_time_us = esp_timer_get_time() + (static_cast<int64_t>(Config::FAN_ON_TIME_MS) * 1000);

        gpio_set_level(Config::FAN_GATE_PIN, 1);
        fan_is_on.store(true);
        current_state.store(FanState::On);

        ESP_LOGI(TAG, "Fan ON (press 1/%lu, %lu ms)",
                 static_cast<unsigned long>(Config::MAX_PRESS_COUNT),
                 static_cast<unsigned long>(Config::FAN_ON_TIME_MS));

        // Blink LED once
        led_controller::blink(1);
    }

    void handle_add_time_command()
    {
        uint8_t current_count = press_count.load();

        if (current_count >= Config::MAX_PRESS_COUNT) {
            ESP_LOGI(TAG, "Max presses reached (%lu), ignoring",
                     static_cast<unsigned long>(Config::MAX_PRESS_COUNT));
            return;
        }

        if (!fan_is_on.load()) {
            // Fan not running, treat as On command
            handle_on_command();
            return;
        }

        // Add 30s to existing timer
        current_count++;
        press_count.store(current_count);
        fan_off_time_us += static_cast<int64_t>(Config::FAN_ON_TIME_MS) * 1000;

        // Calculate remaining time for logging
        int64_t remaining_us = fan_off_time_us - esp_timer_get_time();
        int64_t remaining_ms = remaining_us / 1000;

        ESP_LOGI(TAG, "Added time (press %u/%lu, %lld ms remaining)",
                 current_count,
                 static_cast<unsigned long>(Config::MAX_PRESS_COUNT),
                 remaining_ms);

        // Blink LED N times for N presses
        led_controller::blink(current_count);
    }

    void handle_off_command()
    {
        gpio_set_level(Config::FAN_GATE_PIN, 0);
        fan_is_on.store(false);
        press_count.store(0);
        fan_off_time_us = 0;
        current_state.store(FanState::Off);

        ESP_LOGI(TAG, "Fan OFF (timer reset)");
    }

    void handle_shutdown_command()
    {
        ESP_LOGW(TAG, "Shutdown command received");
        handle_off_command();
        current_state.store(FanState::Shutdown);
    }

    void controller_task(void* pvParameters)
    {
        Config::FanCommand cmd;
        const TickType_t check_interval = pdMS_TO_TICKS(100);

        ESP_LOGI(TAG, "Fan controller task started (max %lu presses, %lu ms each)",
                 static_cast<unsigned long>(Config::MAX_PRESS_COUNT),
                 static_cast<unsigned long>(Config::FAN_ON_TIME_MS));

        while (true) {
            if (xQueueReceive(cmd_queue, &cmd, check_interval) == pdTRUE) {
                switch (cmd) {
                    case Config::FanCommand::On:
                        handle_on_command();
                        break;
                    case Config::FanCommand::AddTime:
                        handle_add_time_command();
                        break;
                    case Config::FanCommand::Off:
                        handle_off_command();
                        break;
                    case Config::FanCommand::Shutdown:
                        handle_shutdown_command();
                        break;
                }
            }

            // Check for auto-off timeout
            if (fan_is_on.load() && fan_off_time_us > 0) {
                int64_t now_us = esp_timer_get_time();

                if (now_us >= fan_off_time_us) {
                    int64_t total_time_ms = static_cast<int64_t>(press_count.load()) * Config::FAN_ON_TIME_MS;
                    ESP_LOGI(TAG, "Auto-off timer expired (%lld ms total)", total_time_ms);
                    handle_off_command();
                }
            }
        }
    }
}

namespace fan_controller {

esp_err_t init()
{
    cmd_queue = xQueueCreate(Config::FAN_QUEUE_SIZE, sizeof(Config::FanCommand));
    if (cmd_queue == nullptr) {
        ESP_LOGE(TAG, "Failed to create fan command queue");
        return ESP_FAIL;
    }

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << Config::FAN_GATE_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure fan GPIO: %s", esp_err_to_name(err));
        return err;
    }

    gpio_set_level(Config::FAN_GATE_PIN, 0);
    fan_is_on.store(false);
    press_count.store(0);
    current_state.store(FanState::Off);

    ESP_LOGI(TAG, "Fan controller initialized on GPIO%d", Config::FAN_GATE_PIN);
    return ESP_OK;
}

void start()
{
    xTaskCreate(
        controller_task,
        "fan_controller",
        Config::FAN_TASK_STACK_SIZE,
        nullptr,
        Config::FAN_TASK_PRIORITY,
        &task_handle
    );
}

esp_err_t send_cmd(Config::FanCommand cmd)
{
    if (cmd_queue != nullptr) {
        BaseType_t result = xQueueSend(cmd_queue, &cmd, 0);
        if (result == pdTRUE) {
            return ESP_OK;
        } else {
            ESP_LOGE(TAG, "Failed to send command to fan controller queue");
            return ESP_FAIL;
        }
    }
    return ESP_FAIL;
}

bool is_on()
{
    return fan_is_on.load();
}

uint8_t get_press_count()
{
    return press_count.load();
}

FanState get_state()
{
    return current_state.load();
}

esp_err_t on()
{
    handle_on_command();
    return ESP_OK;
}

esp_err_t off()
{
    handle_off_command();
    return ESP_OK;
}

} // namespace fan_controller