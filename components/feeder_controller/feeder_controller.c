#include "feeder_controller.h"
#include "driver/ledc.h"
#include "esp_log.h"

static const char* TAG = "FEEDER_DRIVER";

static feeder_config_t s_config;
static volatile bool s_is_feeding = false;

#define FEEDER_LEDC_MODE             LEDC_LOW_SPEED_MODE
#define FEEDER_LEDC_TIMER            LEDC_TIMER_0
#define FEEDER_LEDC_CHANNEL          LEDC_CHANNEL_0
#define FEEDER_LEDC_DUTY_RES         LEDC_TIMER_10_BIT
#define FEEDER_LEDC_DUTY_50_PERCENT  512 // 50% de 1024

void feeder_init(const feeder_config_t *config) {
    s_config = *config;

    // Configura os pinos de controle de direção e enable
    gpio_reset_pin(s_config.dir_pin);
    gpio_set_direction(s_config.dir_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(s_config.dir_pin, 1);

    gpio_reset_pin(s_config.en_pin);
    gpio_set_direction(s_config.en_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(s_config.en_pin, 1); // Desabilitado por padrão (active-low)

    // Configura o timer do LEDC para gerar a frequência dos passos
    uint32_t freq = 1000000 / (2 * s_config.step_delay_us);
    if (freq == 0) {
        freq = 1;
    }
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = FEEDER_LEDC_MODE,
        .timer_num        = FEEDER_LEDC_TIMER,
        .freq_hz          = freq,
        .duty_resolution  = FEEDER_LEDC_DUTY_RES,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    // Configura o canal do LEDC mapeado no pino de STEP
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = FEEDER_LEDC_MODE,
        .channel        = FEEDER_LEDC_CHANNEL,
        .timer_sel      = FEEDER_LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = s_config.step_pin,
        .duty           = 0, // Inicia parado (0% de duty cycle)
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel);

    s_is_feeding = false;
    ESP_LOGI(TAG, "Módulo do motor configurado via LEDC em C.");
}

void feeder_toggle(void) {
    if (s_is_feeding) {
        feeder_stop();
        return;
    }

    s_is_feeding = true;
    gpio_set_level(s_config.en_pin, 0); // Habilita o driver (active-low)
    
    // Inicia a oscilação PWM de 50% de duty cycle para emitir passos
    ledc_set_duty(FEEDER_LEDC_MODE, FEEDER_LEDC_CHANNEL, FEEDER_LEDC_DUTY_50_PERCENT);
    ledc_update_duty(FEEDER_LEDC_MODE, FEEDER_LEDC_CHANNEL);
    
    ESP_LOGI(TAG, "Alimentador ativado via LEDC (Passos iniciados).");
}

void feeder_stop(void) {
    s_is_feeding = false;
    
    // Para a oscilação PWM (0% de duty cycle)
    ledc_set_duty(FEEDER_LEDC_MODE, FEEDER_LEDC_CHANNEL, 0);
    ledc_update_duty(FEEDER_LEDC_MODE, FEEDER_LEDC_CHANNEL);
    
    gpio_set_level(s_config.en_pin, 1); // Desabilita o driver (active-low)
    ESP_LOGW(TAG, "Alimentador desativado.");
}

bool feeder_is_running(void) {
    return s_is_feeding;
}

