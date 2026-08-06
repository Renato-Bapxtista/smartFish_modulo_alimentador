idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "."
    REQUIRES button ds18b20 ph_sensor feeder_controller mq135
)
