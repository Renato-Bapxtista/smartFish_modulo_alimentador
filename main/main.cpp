#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"

// --- NOVOS INCLUDES PARA CORREÇÃO DO WI-FI E REDE ---
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"
// ----------------------------------------------------

#include "button.hpp"
#include "ds18b20.h"
#include "ph_sensor.h"
#include "feeder_controller.h"
#include "mq135.h"
#include "web_server.h"
#include "usecases.h"
#include "adapters.h"
#include "ports.h"

// Sensores declarados como estáticos no escopo de arquivo para acesso seguro entre tarefas
static button::Button s_button(GPIO_NUM_23);
static DS18B20 s_ds18b20(GPIO_NUM_22);
static PhSensor s_ph_sensor;
static Mq135Sensor s_mq135_sensor;

// wafi
#define WIFI_SSID      "Familia Batista"
#define WIFI_PASS      "Cocorico"

static void wifi_init_sta(void) {
    // Cria a interface de rede padrão para modo Estação (Cliente)
    esp_netif_create_default_wifi_sta();
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {};
    // Copia as credenciais de forma segura para a estrutura do ESP-IDF
    strncpy((char*)wifi_config.sta.ssid, WIFI_SSID, sizeof(wifi_config.sta.ssid));
    strncpy((char*)wifi_config.sta.password, WIFI_PASS, sizeof(wifi_config.sta.password));
    
    // Configurações de segurança recomendadas
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    printf("=== Wi-Fi configurado para conectar em: %s ===\n", WIFI_SSID);
    
    // Tenta realizar a conexão inicial
    esp_wifi_connect();
}


// Função auxiliar simples para inicializar o Wi-Fi em modo Access Point (AP) para o Web Server
// Caso seu projeto use modo Estação (conectando no Wi-Fi de casa), ajuste esta função.
static void wifi_init_softap_basic(void) {
    esp_netif_create_default_wifi_ap();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {};
    // Define o nome da rede Wi-Fi gerada pelo ESP32
    uint8_t ssid[] = "ESP32_SmartFit";
    uint8_t password[] = "12345678";
    
    memcpy(wifi_config.ap.ssid, ssid, sizeof(ssid));
    memcpy(wifi_config.ap.password, password, sizeof(password));
    wifi_config.ap.ssid_len = strlen((char*)ssid);
    wifi_config.ap.channel = 1;
    wifi_config.ap.max_connection = 4;
    wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    printf("=== Wi-Fi inicializado em modo SoftAP (SSID: %s) ===\n", ssid);
}

static void telemetry_task(void *pvParameters) {
    printf("=== Iniciando Tarefa de Telemetria (Assíncrona) ===\n");
    while (true) {
        float temperature = 0.0f;
        if (s_ds18b20.readTemperature(&temperature)) {
            printf("DS18B20 temperatura: %.2f °C\n", temperature);
        } else {
            printf("Falha ao ler DS18B20\n");
        }

        float ph_value = 0.0f;
        float ph_temp = 0.0f;
        bool threshold = false;
        if (s_ph_sensor.readPh(&ph_value)) {
            printf("pH: %.2f\n", ph_value);
        } else {
            printf("Falha ao ler pH\n");
        }
        if (s_ph_sensor.readTemperature(&ph_temp)) {
            printf("Temperatura pH: %.2f °C\n", ph_temp);
        }
        if (s_ph_sensor.readThreshold(&threshold)) {
            printf("Threshold Do: %s\n", threshold ? "HIGH" : "LOW");
        }

        float mq135_ppm = 0.0f;
        bool mq135_digital = false;
        if (s_mq135_sensor.readPpm(&mq135_ppm)) {
            printf("MQ135: %.2f ppm\n", mq135_ppm);
        } else {
            printf("Falha ao ler MQ135\n");
        }
        if (s_mq135_sensor.readDigital(&mq135_digital)) {
            printf("MQ135 DO: %s\n", mq135_digital ? "HIGH" : "LOW");
        }

        printf("Feeder está %s\n", feeder_is_running() ? "executando" : "parado");
        printf("--------------------------------------------------\n");

        // Dorme 1 segundo antes da próxima amostragem
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

extern "C" void app_main(void) {
    // -------------------------------------------------------------------------
    // CORREÇÃO CRÍTICA: Inicialização Obrigatória do Subsistema de Rede do ESP-IDF
    // -------------------------------------------------------------------------
    // 1. Inicializa o NVS (necessário para persistir dados do Wi-Fi interno)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. Inicializa a pilha de rede subjacente (Isso resolve diretamente o erro Invalid mbox)
    ESP_ERROR_CHECK(esp_netif_init());

    // 3. Cria o loop de eventos padrão do sistema operacional
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // 4. Inicializa e liga o hardware de Wi-Fi
    wifi_init_sta();
    // -------------------------------------------------------------------------

    // Inicialização do Botão e DS18B20
    s_button.begin();
    s_ds18b20.init();

    // Inicialização do ADC Oneshot compartilhado (ADC_UNIT_1)
    adc_oneshot_unit_handle_t adc1_handle = nullptr;
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
        .clk_src = ADC_RTC_CLK_SRC_DEFAULT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    esp_err_t err = adc_oneshot_new_unit(&init_config, &adc1_handle);
    if (err != ESP_OK) {
        printf("Erro ao inicializar unidade ADC1!\n");
    }

    // Inicialização dos sensores compartilhando o mesmo handle ADC
    s_ph_sensor.init(adc1_handle);
    s_mq135_sensor.init(adc1_handle);

    // Create sensor adapter wrapper and inject into Core class
    class SensorWrapper : public ISensors {
    public:
        SensorWrapper(DS18B20 &d, PhSensor &p, Mq135Sensor &m): d_(d), p_(p), m_(m){}
        virtual bool readTemperature(float *out) override{ return d_.readTemperature(out); }
        virtual bool readPh(float *out) override{ return p_.readPh(out); }
        virtual bool readPpm(float *out) override{ return m_.readPpm(out); }
    private:
        DS18B20 &d_;
        PhSensor &p_;
        Mq135Sensor &m_;
    };

    static SensorWrapper sensors_adapter(s_ds18b20, s_ph_sensor, s_mq135_sensor);
    Core::setSensors(&sensors_adapter);

    // Inicialização do motor de passo via LEDC
    feeder_config_t feeder_config = {
        .step_pin = STEP_GPIO,
        .dir_pin = DIR_GPIO,
        .en_pin = EN_GPIO,
        .steps_per_gram = 200,
        .step_delay_us = 500,
    };
    feeder_init(&feeder_config);

    printf("=== Iniciando leitura do botão e controle do alimentador ===\n");

    // Cria a tarefa de telemetria assíncrona
    xTaskCreate(telemetry_task, "telemetry_task", 4096, NULL, 5, NULL);

    // Inicializa Core e servidores web
    Core::setFeederDriver(create_feeder_adapter());

    // Use SQLite-backed repo/store if available (requires FS mounted and sqlite3 component)
    IFeederRepository* repo = create_sqlite_repo("/spiffs/smartfit.db");
    IConfigStore* cs = create_sqlite_config_store("/spiffs/smartfit.db");
    if(repo && cs){
        Core::setFeederRepository(repo);
        Core::setConfigStore(cs);
    } else {
        // fallback to in-memory
        Core::setFeederRepository(create_inmemory_repo());
        Core::setConfigStore(create_inmemory_config_store());
    }
    Core::init();

    // AGORA o servidor iniciará de maneira estável
    start_web_server();

    button::Button::State previousState = button::Button::State::Released;

    while (true) {
        button::Button::State currentState = s_button.read();
        if (currentState != previousState) {
            if (currentState == button::Button::State::Pressed) {
                if (feeder_is_running()) {
                    feeder_stop();
                    printf(">>> Botão pressionado! Motor parado.\n");
                } else {
                    feeder_toggle();
                    printf(">>> Botão pressionado! Motor em rotação contínua.\n");
                }
            } else {
                printf(">>> Botão solto!\n");
            }
            previousState = currentState;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
