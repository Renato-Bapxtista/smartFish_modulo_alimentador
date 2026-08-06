#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"

#include "button.hpp"
#include "ds18b20.h"
#include "ph_sensor.h"
#include "feeder_controller.h"
#include "mq135.h"

// Sensores declarados como estáticos no escopo de arquivo para acesso seguro entre tarefas
static button::Button s_button(GPIO_NUM_23);
static DS18B20 s_ds18b20(GPIO_NUM_22);
static PhSensor s_ph_sensor;
static Mq135Sensor s_mq135_sensor;

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

        // Dorme 1 segundo antes da próxima amostragem (a leitura do DS18B20 adiciona mais 750ms de bloqueio nesta tarefa)
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

extern "C" void app_main(void) {
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
