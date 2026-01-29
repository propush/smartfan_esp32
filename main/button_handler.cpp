#include "button_handler.hpp"
#include "config.hpp"
#include "fan_controller.hpp"
#include "led_controller.hpp"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

namespace {
    const char* TAG = "button_handler";

    // Event types for button state changes
    enum class ButtonEvent : uint8_t {
        Pressed,   // Falling edge - button pressed down
        Released   // Rising edge - button released
    };

    QueueHandle_t gpio_evt_queue = nullptr;
    TaskHandle_t task_handle = nullptr;

    void IRAM_ATTR gpio_isr_handler(void* arg)
    {
        // Determine event type based on current GPIO level
        // LOW = pressed (falling edge just happened), HIGH = released (rising edge just happened)
        int level = gpio_get_level(BUTTON_PIN);
        ButtonEvent evt = (level == 0) ? ButtonEvent::Pressed : ButtonEvent::Released;
        xQueueSendFromISR(gpio_evt_queue, &evt, nullptr);
    }

    void handler_task(void* pvParameters)
    {
        ButtonEvent evt;
        int64_t press_start_time_us = 0;
        TickType_t last_event_time = 0;

        ESP_LOGI(TAG, "Button handler task started (long press: %lu ms)",
                 static_cast<unsigned long>(LONG_PRESS_TIME_MS));

        while (true) {
            if (xQueueReceive(gpio_evt_queue, &evt, portMAX_DELAY) == pdTRUE) {
                TickType_t current_time = xTaskGetTickCount();

                // Debounce check
                if ((current_time - last_event_time) < pdMS_TO_TICKS(DEBOUNCE_TIME_MS)) {
                    continue;
                }

                // Small delay to let signal settle
                vTaskDelay(pdMS_TO_TICKS(10));

                if (evt == ButtonEvent::Pressed) {
                    // Verify button is actually pressed
                    if (button_handler::is_pressed()) {
                        press_start_time_us = esp_timer_get_time();
                        last_event_time = current_time;
                        ESP_LOGI(TAG, "Button pressed (down)");
                    }
                }
                else if (evt == ButtonEvent::Released) {
                    // Verify button is actually released
                    if (!button_handler::is_pressed() && press_start_time_us > 0) {
                        int64_t press_duration_us = esp_timer_get_time() - press_start_time_us;
                        int64_t press_duration_ms = press_duration_us / 1000;
                        last_event_time = current_time;

                        ESP_LOGI(TAG, "Button released (held %lld ms)", press_duration_ms);

                        if (press_duration_ms >= LONG_PRESS_TIME_MS) {
                            // Long press: stop fan
                            ESP_LOGI(TAG, "Long press detected - stopping fan");
                            fan_controller::send_cmd(FanCommand::Off);
                        } else {
                            // Short press: start or add time
                            if (fan_controller::is_on()) {
                                ESP_LOGI(TAG, "Short press - adding time");
                                fan_controller::send_cmd(FanCommand::AddTime);
                            } else {
                                ESP_LOGI(TAG, "Short press - starting fan");
                                fan_controller::send_cmd(FanCommand::On);
                            }
                        }

                        press_start_time_us = 0;
                    }
                }
            }
        }
    }
}

namespace button_handler {

void init()
{
    gpio_evt_queue = xQueueCreate(BUTTON_QUEUE_SIZE, sizeof(ButtonEvent));
    if (gpio_evt_queue == nullptr) {
        ESP_LOGE(TAG, "Failed to create GPIO event queue");
        return;
    }

    // Configure button pin with internal pull-up
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,    // Use internal pull-up
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE,      // Interrupt on both edges (press and release)
    };
    gpio_config(&io_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_PIN, gpio_isr_handler, reinterpret_cast<void*>(BUTTON_PIN));

    ESP_LOGI(TAG, "Button handler initialized on GPIO%d (internal pull-up)", BUTTON_PIN);
}

void start()
{
    xTaskCreate(
        handler_task,
        "button_handler",
        BUTTON_TASK_STACK_SIZE,
        nullptr,
        BUTTON_TASK_PRIORITY,
        &task_handle
    );
}

bool is_pressed()
{
    return gpio_get_level(BUTTON_PIN) == 0;
}

} // namespace button_handler
