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
        int level = gpio_get_level(Config::BUTTON_PIN);
        ButtonEvent evt = (level == 0) ? ButtonEvent::Pressed : ButtonEvent::Released;
        xQueueSendFromISR(gpio_evt_queue, &evt, nullptr);
    }

    void handler_task(void* pvParameters)
    {
        ButtonEvent evt;
        int64_t press_start_time_us = 0;
        TickType_t last_event_time = 0;

        ESP_LOGI(TAG, "Button handler task started (long press: %lu ms)",
                 static_cast<unsigned long>(Config::LONG_PRESS_TIME_MS));

        while (true) {
            if (xQueueReceive(gpio_evt_queue, &evt, portMAX_DELAY) == pdTRUE) {
                TickType_t current_time = xTaskGetTickCount();

                // Debounce check
                if ((current_time - last_event_time) < pdMS_TO_TICKS(Config::DEBOUNCE_TIME_MS)) {
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

                        if (press_duration_ms >= Config::LONG_PRESS_TIME_MS) {
                            // Long press: stop fan
                            ESP_LOGI(TAG, "Long press detected - stopping fan");
                            esp_err_t err = fan_controller::send_cmd(Config::FanCommand::Off);
                            if (err != ESP_OK) {
                                ESP_LOGW(TAG, "Failed to send off command to fan controller: %s", esp_err_to_name(err));
                            }
                        } else {
                            // Short press: start or add time
                            if (fan_controller::is_on()) {
                                ESP_LOGI(TAG, "Short press - adding time");
                                esp_err_t err = fan_controller::send_cmd(Config::FanCommand::AddTime);
                                if (err != ESP_OK) {
                                    ESP_LOGW(TAG, "Failed to send add time command to fan controller: %s", esp_err_to_name(err));
                                }
                            } else {
                                ESP_LOGI(TAG, "Short press - starting fan");
                                esp_err_t err = fan_controller::send_cmd(Config::FanCommand::On);
                                if (err != ESP_OK) {
                                    ESP_LOGW(TAG, "Failed to send on command to fan controller: %s", esp_err_to_name(err));
                                }
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

esp_err_t init()
{
    // Check if queue is already initialized
    if (gpio_evt_queue != nullptr) {
        ESP_LOGW(TAG, "Button handler already initialized");
        return ESP_OK;
    }

    gpio_evt_queue = xQueueCreate(Config::BUTTON_QUEUE_SIZE, sizeof(ButtonEvent));
    if (gpio_evt_queue == nullptr) {
        ESP_LOGE(TAG, "Failed to create GPIO event queue");
        return ESP_FAIL;
    }

    // Configure button pin with internal pull-up
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << Config::BUTTON_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,    // Use internal pull-up
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE,      // Interrupt on both edges (press and release)
    };
    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure button GPIO: %s", esp_err_to_name(err));
        return err;
    }

    err = gpio_install_isr_service(0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install GPIO ISR service: %s", esp_err_to_name(err));
        return err;
    }

    err = gpio_isr_handler_add(Config::BUTTON_PIN, gpio_isr_handler, reinterpret_cast<void*>(Config::BUTTON_PIN));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add GPIO ISR handler: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "Button handler initialized on GPIO%d (internal pull-up)", Config::BUTTON_PIN);
    return ESP_OK;
}

void start()
{
    if (gpio_evt_queue == nullptr) {
        ESP_LOGE(TAG, "Button handler not initialized, cannot start task");
        return;
    }

    xTaskCreate(
        handler_task,
        "button_handler",
        Config::BUTTON_TASK_STACK_SIZE,
        nullptr,
        Config::BUTTON_TASK_PRIORITY,
        &task_handle
    );
}

bool is_pressed()
{
    return gpio_get_level(Config::BUTTON_PIN) == 0;
}

} // namespace button_handler
