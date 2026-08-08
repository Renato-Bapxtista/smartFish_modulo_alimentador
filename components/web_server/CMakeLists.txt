

idf_component_register(SRCS "web_server.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_http_server ds18b20 ph_sensor feeder_controller
                       EMBED_FILES "../web_ui/www/index.html"
                                   "../web_ui/www/style.css"
                                   "../web_ui/www/app.js")

