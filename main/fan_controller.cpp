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
    int64_t fan_off_time_us = 0;  // Absolute time when fan should turn off

    void handle_on_command()
    {
        // First press: start fan with 30s timer
        press_count.store(1);
        fan_off_time_us = esp_timer_get_time() + (static_cast<int64_t>(FAN_ON_TIME_MS) * 1000);

        gpio_set_level(FAN_GATE_PIN, 1);
        fan_is_on.store(true);

        ESP_LOGI(TAG, "Fan ON (press 1/%lu, %lu ms)",
                 static_cast<unsigned long>(MAX_PRESS_COUNT),
                 static_cast<unsigned long>(FAN_ON_TIME_MS));

        // Blink LED once
        led_controller::blink(1);
    }

    void handle_add_time_command()
    {
        uint8_t current_count = press_count.load();

        if (current_count >= MAX_PRESS_COUNT) {
            ESP_LOGI(TAG, "Max presses reached (%lu), ignoring",
                     static_cast<unsigned long>(MAX_PRESS_COUNT));
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
        fan_off_time_us += static_cast<int64_t>(FAN_ON_TIME_MS) * 1000;

        // Calculate remaining time for logging
        int64_t remaining_us = fan_off_time_us - esp_timer_get_time();
        int64_t remaining_ms = remaining_us / 1000;

        ESP_LOGI(TAG, "Added time (press %u/%lu, %lld ms remaining)",
                 current_count,
                 static_cast<unsigned long>(MAX_PRESS_COUNT),
                 remaining_ms);

        // Blink LED N times for N presses
        led_controller::blink(current_count);
    }

    void handle_off_command()
    {
        gpio_set_level(FAN_GATE_PIN, 0);
        fan_is_on.store(false);
        press_count.store(0);
        fan_off_time_us = 0;

        ESP_LOGI(TAG, "Fan OFF (timer reset)");
    }

    void controller_task(void* pvParameters)
    {
        FanCommand cmd;
        const TickType_t check_interval = pdMS_TO_TICKS(100);

        ESP_LOGI(TAG, "Fan controller task started (max %lu presses, %lu ms each)",
                 static_cast<unsigned long>(MAX_PRESS_COUNT),
                 static_cast<unsigned long>(FAN_ON_TIME_MS));

        while (true) {
            if (xQueueReceive(cmd_queue, &cmd, check_interval) == pdTRUE) {
                switch (cmd) {
                    case FanCommand::On:
                        handle_on_command();
                        break;
                    case FanCommand::AddTime:
                        handle_add_time_command();
                        break;
                    case FanCommand::Off:
                        handle_off_command();
                        break;
                    case FanCommand::Shutdown:
                        ESP_LOGW(TAG, "Shutdown command received");
                        handle_off_command();
                        break;
                }
            }

            // Check for auto-off timeout
            if (fan_is_on.load() && fan_off_time_us > 0) {
                int64_t now_us = esp_timer_get_time();

                if (now_us >= fan_off_time_us) {
                    int64_t total_time_ms = static_cast<int64_t>(press_count.load()) * FAN_ON_TIME_MS;
                    ESP_LOGI(TAG, "Auto-off timer expired (%lld ms total)", total_time_ms);
                    handle_off_command();
                }
            }
        }
    }
}

namespace fan_controller {

void init()
{
    cmd_queue = xQueueCreate(FAN_QUEUE_SIZE, sizeof(FanCommand));
    if (cmd_queue == nullptr) {
        ESP_LOGE(TAG, "Failed to create fan command queue");
        return;
    }

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << FAN_GATE_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    gpio_set_level(FAN_GATE_PIN, 0);
    fan_is_on.store(false);
    press_count.store(0);

    ESP_LOGI(TAG, "Fan controller initialized on GPIO%d", FAN_GATE_PIN);
}

void start()
{
    xTaskCreate(
        controller_task,
        "fan_controller",
        FAN_TASK_STACK_SIZE,
        nullptr,
        FAN_TASK_PRIORITY,
        &task_handle
    );
}

void send_cmd(FanCommand cmd)
{
    if (cmd_queue != nullptr) {
        xQueueSend(cmd_queue, &cmd, 0);
    }
}

bool is_on()
{
    return fan_is_on.load();
}

uint8_t get_press_count()
{
    return press_count.load();
}

void on()
{
    handle_on_command();
}

void off()
{
    handle_off_command();
}

} // namespace fan_controller
