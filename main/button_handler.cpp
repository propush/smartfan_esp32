#include "button_handler.hpp"
#include "config.hpp"
#include "fan_controller.hpp"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

namespace {
    const char* TAG = "button_handler";

    QueueHandle_t gpio_evt_queue = nullptr;
    TaskHandle_t task_handle = nullptr;

    void IRAM_ATTR gpio_isr_handler(void* arg)
    {
        uint32_t gpio_num = reinterpret_cast<uint32_t>(arg);
        xQueueSendFromISR(gpio_evt_queue, &gpio_num, nullptr);
    }

    void handler_task(void* pvParameters)
    {
        uint32_t gpio_num;
        TickType_t last_press_time = 0;

        ESP_LOGI(TAG, "Button handler task started");

        while (true) {
            if (xQueueReceive(gpio_evt_queue, &gpio_num, portMAX_DELAY) == pdTRUE) {
                TickType_t current_time = xTaskGetTickCount();

                if ((current_time - last_press_time) >= pdMS_TO_TICKS(DEBOUNCE_TIME_MS)) {
                    vTaskDelay(pdMS_TO_TICKS(10));
                    if (button_handler::is_pressed()) {
                        ESP_LOGI(TAG, "Button pressed");
                        last_press_time = current_time;
                        fan_controller::send_cmd(FanCommand::On);
                    }
                }
            }
        }
    }
}

namespace button_handler {

void init()
{
    gpio_evt_queue = xQueueCreate(BUTTON_QUEUE_SIZE, sizeof(uint32_t));
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
        .intr_type = GPIO_INTR_NEGEDGE,      // Interrupt on falling edge (button press)
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
