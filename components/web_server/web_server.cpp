#include <esp_http_server.h>
#include "esp_log.h"
#include "feeder_controller.h"
#include "ds18b20.h"
#include "ph_sensor.h"

static const char *TAG = "smartfish_web";

// Instancia o sensor DS18B20 utilizando o GPIO padrão configurado na classe (GPIO 4)
static DS18B20 ds18b20_sensor(GPIO_NUM_4);


// Mapeamento dos binários gerados pelo CMake (baseado no caminho relativo do componente)
extern "C" const uint8_t index_html_start[] asm("_binary_index_html_start");
extern "C" const uint8_t index_html_end[]   asm("_binary_index_html_end");

extern "C" const uint8_t style_css_start[]  asm("_binary_style_css_start");
extern "C" const uint8_t style_css_end[]    asm("_binary_style_css_end");

extern "C" const uint8_t app_js_start[]     asm("_binary_app_js_start");
extern "C" const uint8_t app_js_end[]       asm("_binary_app_js_end");


// Handler: Página Principal (index.html)
static esp_err_t index_get_handler(httpd_req_t *req) {
    const size_t size = (index_html_end - index_html_start);
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_send(req, (const char *)index_html_start, size);
    return ESP_OK;
}

// Handler: Estilos (style.css)
static esp_err_t css_get_handler(httpd_req_t *req) {
    const size_t size = (style_css_end - style_css_start);
    httpd_resp_set_type(req, "text/css");
    httpd_resp_send(req, (const char *)style_css_start, size);
    return ESP_OK;
}

// Handler: Scripts (app.js)
static esp_err_t js_get_handler(httpd_req_t *req) {
    const size_t size = (app_js_end - app_js_start);
    httpd_resp_set_type(req, "application/javascript");
    httpd_resp_send(req, (const char *)app_js_start, size);
    return ESP_OK;
}

// Handler para fornecer os dados dos sensores via JSON utilizando a classe C++
static esp_err_t api_status_get_handler(httpd_req_t *req) {
    float temperatura = 0.0f;
    float ph_valor = 7.0f; 

    bool leitura_ok = ds18b20_sensor.readTemperature(&temperatura);
    if (!leitura_ok) {
        ESP_LOGW(TAG, "Falha ao ler a temperatura do sensor DS18B20");
    }

    char json_response[100];
    snprintf(json_response, sizeof(json_response), "{\"temperature\":%.2f,\"ph\":%.2f}", temperatura, ph_valor);
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_response, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// Handler para acionar a dosagem de ração
static esp_err_t api_feed_post_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "Acionando motor de passo para alimentação via Web...");
    
    // Insira aqui a função do seu driver de motor, ex: feeder_trigger();
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"status\":\"success\"}", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// Inicialização do servidor web e registro de rotas
void start_smartfish_server(void) {
    ds18b20_sensor.init();

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 10; // Aumentado para suportar as novas rotas de arquivos assets

    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        // Rota: HTML principal
        httpd_uri_t index_uri = { .uri = "/", .method = HTTP_GET, .handler = index_get_handler, .user_ctx  = NULL };
        httpd_register_uri_handler(server, &index_uri);

        // Rota: Arquivo CSS
        httpd_uri_t css_uri = { .uri = "/style.css", .method = HTTP_GET, .handler = css_get_handler, .user_ctx  = NULL };
        httpd_register_uri_handler(server, &css_uri);

        // Rota: Arquivo Javascript
        httpd_uri_t js_uri = { .uri = "/app.js", .method = HTTP_GET, .handler = js_get_handler, .user_ctx  = NULL };
        httpd_register_uri_handler(server, &js_uri);

        // Rota API: Status dos sensores
        httpd_uri_t status_uri = { .uri = "/api/status", .method = HTTP_GET, .handler = api_status_get_handler, .user_ctx  = NULL };
        httpd_register_uri_handler(server, &status_uri);

        // Rota API: Comando de alimentação
        httpd_uri_t feed_uri = { .uri = "/api/feed", .method = HTTP_POST, .handler = api_feed_post_handler, .user_ctx  = NULL };
        httpd_register_uri_handler(server, &feed_uri);

        ESP_LOGI(TAG, "Servidor Web iniciado com sucesso!");
    } else {
        ESP_LOGE(TAG, "Falha ao iniciar o servidor Web.");
    }
}
