#include "fan_controller.hpp"
#include "config.hpp"
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
    int64_t fan_on_time_us = 0;

    void controller_task(void* pvParameters)
    {
        FanCommand cmd;
        const TickType_t check_interval = pdMS_TO_TICKS(100);

        ESP_LOGI(TAG, "Fan controller task started (auto-off after %lu ms)",
                 static_cast<unsigned long>(FAN_ON_TIME_MS));

        while (true) {
            if (xQueueReceive(cmd_queue, &cmd, check_interval) == pdTRUE) {
                switch (cmd) {
                    case FanCommand::On:
                        fan_controller::on();
                        break;
                    case FanCommand::Off:
                        fan_controller::off();
                        break;
                    case FanCommand::Shutdown:
                        ESP_LOGW(TAG, "Shutdown command received");
                        fan_controller::off();
                        break;
                }
            }

            // Check for auto-off timeout
            if (fan_is_on.load()) {
                int64_t elapsed_us = esp_timer_get_time() - fan_on_time_us;
                int64_t timeout_us = static_cast<int64_t>(FAN_ON_TIME_MS) * 1000;

                if (elapsed_us >= timeout_us) {
                    ESP_LOGI(TAG, "Auto-off timer expired (%lu ms)",
                             static_cast<unsigned long>(FAN_ON_TIME_MS));
                    fan_controller::off();
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

void on()
{
    gpio_set_level(FAN_GATE_PIN, 1);
    fan_is_on.store(true);
    fan_on_time_us = esp_timer_get_time();
    ESP_LOGI(TAG, "Fan ON");
}

void off()
{
    gpio_set_level(FAN_GATE_PIN, 0);
    fan_is_on.store(false);
    ESP_LOGI(TAG, "Fan OFF");
}

} // namespace fan_controller
