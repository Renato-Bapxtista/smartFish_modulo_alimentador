#include "ph_sensor.h"

#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>

static const char *TAG = "PH_SENSOR";

#define PH_ADC_CHANNEL_PH   ADC_CHANNEL_6
#define PH_ADC_CHANNEL_TEMP ADC_CHANNEL_0
#define PH_ADC_ATTEN        ADC_ATTEN_DB_12
#define PH_ADC_WIDTH        ADC_BITWIDTH_12
#define PH_ADC_SAMPLES      8
#define PH_DEFAULT_VREF     1100
#define PH_ADC_MAX_FLUCT_RAW 120

#define PH_CALIBRATION_VOLTAGE_MV 2510.0f
#define PH_CALIBRATION_PH          7.0f
#define PH_SLOPE_MV_PER_PH        59.16f

#define TEMP_DEGREES_PER_MV          0.006f
#define TEMP_DEGREE_OFFSET          -2.0f

PhSensor::PhSensor(gpio_num_t po_gpio, gpio_num_t to_gpio, gpio_num_t do_gpio)
    : po_gpio_(po_gpio),
      to_gpio_(to_gpio),
      do_gpio_(do_gpio),
      initialized_(false),
      adc_handle_(nullptr),
      cali_handle_(nullptr) {}

PhSensor::~PhSensor()
{
    cleanupCalibration();
}

bool PhSensor::init(adc_oneshot_unit_handle_t adc_handle)
{
    if (initialized_) {
        ESP_LOGW(TAG, "pH sensor already initialized");
        return true;
    }

    if (adc_handle == nullptr) {
        ESP_LOGE(TAG, "Invalid ADC handle provided");
        return false;
    }
    adc_handle_ = adc_handle;

    if (!configureChannels()) {
        return false;
    }

    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << do_gpio_);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;

    if (gpio_config(&io_conf) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure Do (GPIO input)");
        cleanupCalibration();
        return false;
    }

    ESP_LOGI(TAG, "pH sensor initialized:");
    ESP_LOGI(TAG, "  Po (pH):        ADC1_CH6 (GPIO%d)", po_gpio_);
    ESP_LOGI(TAG, "  To (Temp):      ADC1_CH0 (GPIO%d)", to_gpio_);
    ESP_LOGI(TAG, "  Do (Threshold): GPIO%d (digital input)", do_gpio_);

    initialized_ = true;
    return true;
}

bool PhSensor::configureChannels()
{
    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = PH_ADC_ATTEN,
        .bitwidth = PH_ADC_WIDTH,
    };

    if (adc_oneshot_config_channel(adc_handle_, PH_ADC_CHANNEL_PH, &chan_cfg) != ESP_OK ||
        adc_oneshot_config_channel(adc_handle_, PH_ADC_CHANNEL_TEMP, &chan_cfg) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure ADC channels");
        cleanupCalibration();
        return false;
    }

    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT_1,
        .atten = PH_ADC_ATTEN,
        .bitwidth = PH_ADC_WIDTH,
        .default_vref = PH_DEFAULT_VREF,
    };

    if (adc_cali_create_scheme_line_fitting(&cali_config, &cali_handle_) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create ADC calibration scheme");
        cleanupCalibration();
        return false;
    }

    return true;
}

bool PhSensor::cleanupCalibration()
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

bool PhSensor::readAdcVoltageMv(adc_channel_t channel, int *voltage_mv)
{
    if (!initialized_ || voltage_mv == nullptr || adc_handle_ == nullptr || cali_handle_ == nullptr) {
        return false;
    }

    int min_raw = std::numeric_limits<int>::max();
    int max_raw = 0;
    int sum_raw = 0;

    for (int i = 0; i < PH_ADC_SAMPLES; ++i) {
        int raw = 0;
        if (adc_oneshot_read(adc_handle_, channel, &raw) != ESP_OK) {
            ESP_LOGW(TAG, "ADC read failed on channel %d", channel);
            return false;
        }
        if (raw < 0) {
            raw = 0;
        }
        sum_raw += raw;
        min_raw = std::min(min_raw, raw);
        max_raw = std::max(max_raw, raw);
        esp_rom_delay_us(100);
    }

    if ((max_raw - min_raw) > PH_ADC_MAX_FLUCT_RAW) {
        ESP_LOGW(TAG, "ADC readings unstable on channel %d (min=%d max=%d)", channel, min_raw, max_raw);
        return false;
    }

    int avg_raw = sum_raw / PH_ADC_SAMPLES;
    int voltage = 0;
    if (adc_cali_raw_to_voltage(cali_handle_, avg_raw, &voltage) != ESP_OK) {
        ESP_LOGW(TAG, "Failed to convert ADC raw to voltage");
        return false;
    }

    *voltage_mv = voltage;
    return true;
}

float PhSensor::convertVoltageToPh(int voltage_mv)
{
    return PH_CALIBRATION_PH + (voltage_mv - PH_CALIBRATION_VOLTAGE_MV) / PH_SLOPE_MV_PER_PH;
}

float PhSensor::convertVoltageToTemp(int voltage_mv)
{
    return voltage_mv * TEMP_DEGREES_PER_MV + TEMP_DEGREE_OFFSET;
}

bool PhSensor::readPh(float *ph_value)
{
    if (!initialized_ || ph_value == nullptr) {
        return false;
    }

    int voltage_mv = 0;
    if (!readAdcVoltageMv(PH_ADC_CHANNEL_PH, &voltage_mv)) {
        return false;
    }

    *ph_value = convertVoltageToPh(voltage_mv);
    return true;
}

bool PhSensor::readTemperature(float *temperature)
{
    if (!initialized_ || temperature == nullptr) {
        return false;
    }

    int voltage_mv = 0;
    if (!readAdcVoltageMv(PH_ADC_CHANNEL_TEMP, &voltage_mv)) {
        return false;
    }

    *temperature = convertVoltageToTemp(voltage_mv);
    return true;
}

bool PhSensor::readThreshold(bool *triggered)
{
    if (!initialized_ || triggered == nullptr) {
        return false;
    }

    int level = gpio_get_level(do_gpio_);
    *triggered = (level == 1);
    return true;
}

bool PhSensor::readAll(PhSensorData *data)
{
    if (!initialized_ || data == nullptr) {
        return false;
    }

    if (!readPh(&data->ph_value)) {
        data->ph_value = NAN;
    }
    if (!readTemperature(&data->temperature)) {
        data->temperature = NAN;
    }
    readThreshold(&data->threshold_triggered);
    return true;
}
