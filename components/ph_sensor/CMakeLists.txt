idf_component_register(
    SRCS "ph_sensor.cpp"
    INCLUDE_DIRS "include"
    REQUIRES esp_adc esp_driver_gpio freertos log
)
