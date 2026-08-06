idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "."
    REQUIRES nvs_flash esp_wifi esp_event esp_netif core button ds18b20 ph_sensor feeder_controller mq135 web_server
)
