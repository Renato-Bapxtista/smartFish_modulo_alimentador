idf_component_register(
    SRCS "mq135.cpp"
    INCLUDE_DIRS "include"
    REQUIRES esp_adc esp_driver_gpio freertos log
)
