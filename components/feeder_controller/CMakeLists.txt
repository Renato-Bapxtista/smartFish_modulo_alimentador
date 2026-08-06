idf_component_register(
    SRCS "feeder_controller.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_driver_gpio esp_driver_ledc freertos log
)
