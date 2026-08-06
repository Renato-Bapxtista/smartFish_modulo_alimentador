#include "ds18b20.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_rom_sys.h"
#include "esp_log.h"

static const char *TAG = "DS18B20";
static portMUX_TYPE ds18b20_mux = portMUX_INITIALIZER_UNLOCKED;

DS18B20::DS18B20(gpio_num_t gpio)
    : gpio_(gpio) {}

bool DS18B20::init()
{
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << gpio_,
        .mode = GPIO_MODE_INPUT_OUTPUT_OD,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    gpio_set_level(gpio_, 1);

    esp_rom_delay_us(100);
    if (gpio_get_level(gpio_) == 0) {
        ESP_LOGW(TAG, "1-Wire line not pulled high — relying on internal pull-up");
    }
    return true;
}

bool DS18B20::readTemperature(float *temperature)
{
    uint8_t scratchpad[9];

    if (!startConversion()) {
        ESP_LOGW(TAG, "Failed to start temperature conversion");
        return false;
    }

    vTaskDelay(pdMS_TO_TICKS(750));

    if (!reset()) {
        ESP_LOGW(TAG, "DS18B20 reset failed before scratchpad read");
        return false;
    }

    writeByte(0xCC);
    writeByte(0xBE);

    for (int i = 0; i < 9; i++) {
        scratchpad[i] = readByte();
    }

    if (crc8(scratchpad, sizeof(scratchpad)) != 0) {
        ESP_LOGW(TAG, "CRC check failed");
        return false;
    }

    int16_t raw = static_cast<int16_t>((scratchpad[1] << 8) | scratchpad[0]);
    *temperature = raw / 16.0f;
    return true;
}

void DS18B20::driveLow()
{
    gpio_set_level(gpio_, 0);
}

void DS18B20::release()
{
    gpio_set_level(gpio_, 1);
}

bool DS18B20::reset()
{
    portENTER_CRITICAL(&ds18b20_mux);
    driveLow();
    esp_rom_delay_us(480);
    release();

    // Espera a linha subir para nível alto (máximo de 100us) para compensar pull-ups fracos
    int wait_high_us = 100;
    while (gpio_get_level(gpio_) == 0 && wait_high_us > 0) {
        esp_rom_delay_us(1);
        wait_high_us--;
    }

    if (wait_high_us == 0) {
        portEXIT_CRITICAL(&ds18b20_mux);
        ESP_LOGD(TAG, "DS18B20 reset: bus stuck low");
        return false;
    }

    // Agora espera o sensor puxar a linha para baixo (pulso de presença, até 70us)
    int wait_presence_us = 70;
    int presence = 1;
    while (wait_presence_us > 0) {
        if (gpio_get_level(gpio_) == 0) {
            presence = 0;
            break;
        }
        esp_rom_delay_us(1);
        wait_presence_us--;
    }

    // Aguarda o restante do tempo de recuperação (410us total)
    esp_rom_delay_us(410);
    portEXIT_CRITICAL(&ds18b20_mux);

    if (presence != 0) {
        ESP_LOGD(TAG, "DS18B20 presence pulse not detected");
    }
    return (presence == 0);
}

void DS18B20::writeBit(bool bit)
{
    portENTER_CRITICAL(&ds18b20_mux);
    driveLow();
    esp_rom_delay_us(6);
    if (bit) {
        release();
        esp_rom_delay_us(64);
    } else {
        esp_rom_delay_us(60);
        release();
        esp_rom_delay_us(10);
    }
    portEXIT_CRITICAL(&ds18b20_mux);
}

bool DS18B20::readBit()
{
    portENTER_CRITICAL(&ds18b20_mux);
    driveLow();
    esp_rom_delay_us(2); // Reduzido de 6us para 2us para dar mais tempo de subida em pull-ups fracos
    release();
    esp_rom_delay_us(8); // Amostra com 10us desde o início do slot

    bool bit = gpio_get_level(gpio_);
    esp_rom_delay_us(55);
    portEXIT_CRITICAL(&ds18b20_mux);
    return bit;
}

void DS18B20::writeByte(uint8_t value)
{
    for (int i = 0; i < 8; i++) {
        writeBit((value >> i) & 0x01);
    }
}

uint8_t DS18B20::readByte()
{
    uint8_t value = 0;
    for (int i = 0; i < 8; i++) {
        if (readBit()) {
            value |= static_cast<uint8_t>(1 << i);
        }
    }
    return value;
}

uint8_t DS18B20::crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0;
    while (len--) {
        uint8_t inbyte = *data++;
        for (int i = 0; i < 8; i++) {
            uint8_t mix = (crc ^ inbyte) & 0x01;
            crc >>= 1;
            if (mix) {
                crc ^= 0x8C;
            }
            inbyte >>= 1;
        }
    }
    return crc;
}

bool DS18B20::startConversion()
{
    const int max_attempts = 3;
    for (int attempt = 0; attempt < max_attempts; ++attempt) {
        if (reset()) {
            writeByte(0xCC);
            writeByte(0x44);
            return true;
        }
        ESP_LOGD(TAG, "ds18b20_start_conversion: reset attempt %d failed", attempt + 1);
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    return false;
}
