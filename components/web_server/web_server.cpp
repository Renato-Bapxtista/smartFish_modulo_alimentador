#include <esp_http_server.h>
#include "esp_log.h"
#include "feeder_controller.h"
#include "ds18b20.h"
#include "ph_sensor.h"

static const char *TAG = "smartfish_web";

// Instancia o sensor DS18B20 utilizando o GPIO padrão configurado na classe (GPIO 4)
static DS18B20 ds18b20_sensor(GPIO_NUM_4);

// Handler da Página Principal (Dashboard Web do SmartFish)
static esp_err_t index_get_handler(httpd_req_t *req) {
    const char* html_dashboard = 
        "<!DOCTYPE html><html><head><meta charset='utf-8'><title>SmartFish</title>"
        "<style>"
        "body{font-family:Arial,sans-serif;background:#f0f2f5;text-align:center;padding:30px;}"
        ".card{background:white;padding:25px;border-radius:12px;box-shadow:0 4px 12px rgba(0,0,0,0.1);display:inline-block;max-width:400px;width:100%;}"
        "h2{color:#007bff;margin-top:0;}"
        ".metric{font-size:18px;margin:15px 0;background:#f8f9fa;padding:10px;border-radius:8px;}"
        "span{font-weight:bold;color:#333;}"
        "button{background:#007bff;color:white;border:none;padding:12px 24px;border-radius:6px;cursor:pointer;font-size:16px;width:100%;margin-top:10px;}"
        "button:hover{background:#0056b3;}"
        "</style></head><body>"
        "<div class='card'>"
        "<h2>SmartFish Dashboard</h2>"
        "<div class='metric'>Temperatura: <span id='temp'>--</span> °C</div>"
        "<div class='metric'>pH da Água: <span id='ph'>--</span></div>"
        "<button onclick='alimentar()'>Alimentar Agora</button>"
        "</div>"
        "<script>"
        "function atualizarDados() {"
        "  fetch('/api/status').then(res => res.json()).then(data => {"
        "    document.getElementById('temp').innerText = data.temperature.toFixed(1);"
        "    document.getElementById('ph').innerText = data.ph.toFixed(2);"
        "  }).catch(err => console.log('Erro ao buscar dados', err));"
        "}"
        "function alimentar() {"
        "  fetch('/api/feed', {method: 'POST'})"
        "    .then(r => r.json())"
        "    .then(data => alert('Comando enviado! Status: ' + data.status))"
        "    .catch(err => alert('Erro ao acionar alimentador'));"
        "}"
        "setInterval(atualizarDados, 3000);"
        "atualizarDados();"
        "</script></body></html>";

    httpd_resp_send(req, html_dashboard, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// Handler para fornecer os dados dos sensores via JSON utilizando a classe C++
static esp_err_t api_status_get_handler(httpd_req_t *req) {
    float temperatura = 0.0f;
    float ph_valor = 7.0f; 

    // Chamada correta do método orientado a objetos da sua classe DS18B20
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
    // Inicializa o sensor DS18B20 no boot do servidor (conforme o método init da classe)
    ds18b20_sensor.init();

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t index_uri = { 
            .uri = "/", 
            .method = HTTP_GET, 
            .handler = index_get_handler, 
            .user_ctx = NULL 
        };
        httpd_register_uri_handler(server, &index_uri);

        httpd_uri_t status_uri = { 
            .uri = "/api/status", 
            .method = HTTP_GET, 
            .handler = api_status_get_handler, 
            .user_ctx = NULL 
        };
        httpd_register_uri_handler(server, &status_uri);

        httpd_uri_t feed_uri = { 
            .uri = "/api/feed", 
            .method = HTTP_POST, 
            .handler = api_feed_post_handler, 
            .user_ctx = NULL 
        };
        httpd_register_uri_handler(server, &feed_uri);
        
        ESP_LOGI(TAG, "Servidor Web iniciado com sucesso!");
    }
}