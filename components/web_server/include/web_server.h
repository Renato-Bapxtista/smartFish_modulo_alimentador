#pragma once
#include "esp_http_server.h"

/* Inicia o servidor web e registra as rotas. Retorna handle do servidor. */
httpd_handle_t start_web_server(void);
