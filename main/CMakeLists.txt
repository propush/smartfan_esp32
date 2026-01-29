idf_component_register(
    SRCS
        "main.cpp"
        "power_manager.cpp"
        "adc_monitor.cpp"
        "button_handler.cpp"
        "fan_controller.cpp"
        "led_controller.cpp"
    INCLUDE_DIRS "."
    REQUIRES
        driver
        esp_adc
        nvs_flash
        esp_timer
)
