#include "mq135.h"

#include "driver/gpio.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

#include <algorithm>

static const char *TAG = "MQ135";

Mq135Sensor::Mq135Sensor(gpio_num_t do_gpio, gpio_num_t ao_gpio, adc_channel_t channel)
    : do_gpio_(do_gpio),
      ao_gpio_(ao_gpio),
      channel_(channel),
      initialized_(false),
      adc_handle_(nullptr),
      cali_handle_(nullptr) {}

Mq135Sensor::~Mq135Sensor()
{
    cleanupCalibration();
}

bool Mq135Sensor::init(adc_oneshot_unit_handle_t adc_handle)
{
    if (initialized_) {
        ESP_LOGW(TAG, "MQ135 sensor already initialized");
        return true;
    }

    if (adc_handle == nullptr) {
        ESP_LOGE(TAG, "Invalid ADC handle provided");
        return false;
    }
    adc_handle_ = adc_handle;

    if (!configureChannel()) {
        return false;
    }

    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << do_gpio_);
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;

    if (gpio_config(&io_conf) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure DO pin GPIO%d", do_gpio_);
        cleanupCalibration();
        return false;
    }

    ESP_LOGI(TAG, "MQ135 sensor initialized: DO on GPIO%d, AO on GPIO%d, ADC channel %d", do_gpio_, ao_gpio_, channel_);
    initialized_ = true;
    return true;
}

bool Mq135Sensor::configureChannel()
{
    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };

    if (adc_oneshot_config_channel(adc_handle_, channel_, &chan_cfg) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure ADC channel %d", channel_);
        cleanupCalibration();
        return false;
    }

    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT_1,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
        .default_vref = 1100,
    };

    if (adc_cali_create_scheme_line_fitting(&cali_config, &cali_handle_) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create ADC calibration scheme");
        cleanupCalibration();
        return false;
    }

    return true;
}

bool Mq135Sensor::cleanupCalibration()
{
    bool ok = true;

    if (cali_handle_ != nullptr) {
        if (adc_cali_delete_scheme_line_fitting(cali_handle_) != ESP_OK) {
            ok = false;
        }
        cali_handle_ = nullptr;
    }

    adc_handle_ = nullptr;
    initialized_ = false;
    return ok;
}

bool Mq135Sensor::readRaw(int *raw_value)
{
    if (!initialized_ || raw_value == nullptr) {
        return false;
    }

    int raw = 0;
    if (adc_oneshot_read(adc_handle_, channel_, &raw) != ESP_OK) {
        ESP_LOGW(TAG, "Failed to read ADC channel %d", channel_);
        return false;
    }

    *raw_value = raw;
    return true;
}

bool Mq135Sensor::readVoltageMv(int *voltage_mv)
{
    if (!initialized_ || voltage_mv == nullptr || adc_handle_ == nullptr || cali_handle_ == nullptr) {
        return false;
    }

    int raw = 0;
    if (!readRaw(&raw)) {
        return false;
    }

    int voltage = 0;
    if (adc_cali_raw_to_voltage(cali_handle_, raw, &voltage) != ESP_OK) {
        ESP_LOGW(TAG, "Failed to convert ADC raw value to voltage");
        return false;
    }

    *voltage_mv = voltage;
    return true;
}

bool Mq135Sensor::readPpm(float *ppm_value)
{
    if (!initialized_ || ppm_value == nullptr) {
        return false;
    }

    int voltage_mv = 0;
    if (!readVoltageMv(&voltage_mv)) {
        return false;
    }

    const float baseline_mv = 400.0f;
    const float mv_per_ppm = 2.0f;
    const float estimated_ppm = std::max(0.0f, (voltage_mv - baseline_mv) / mv_per_ppm);

    *ppm_value = estimated_ppm;
    return true;
}

bool Mq135Sensor::readDigital(bool *state)
{
    if (!initialized_ || state == nullptr) {
        return false;
    }

    *state = (gpio_get_level(do_gpio_) == 1);
    return true;
}
