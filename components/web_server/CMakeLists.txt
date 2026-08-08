
idf_component_register(
    SRCS "web_server.cpp"
    INCLUDE_DIRS "include"
    REQUIRES feeder_controller ds18b20 ph_sensor esp_http_server
)