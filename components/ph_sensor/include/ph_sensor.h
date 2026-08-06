#ifndef PH_SENSOR_H_
#define PH_SENSOR_H_

#include <cstdint>
#include <cstddef>
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"

struct PhSensorData {
    float ph_value;
    float temperature;
    bool threshold_triggered;
};

class PhSensor {
public:
    explicit PhSensor(gpio_num_t po_gpio = GPIO_NUM_34,     // pH analog input (ADC1_CH6)
                      gpio_num_t to_gpio = GPIO_NUM_36,     // Temp analog input (ADC1_CH0)
                      gpio_num_t do_gpio = GPIO_NUM_32);     // Digital threshold input
    ~PhSensor();

    bool init(adc_oneshot_unit_handle_t adc_handle);
    bool readPh(float *ph_value);
    bool readTemperature(float *temperature);
    bool readThreshold(bool *triggered);
    bool readAll(PhSensorData *data);

    PhSensor(const PhSensor &) = delete;
    PhSensor &operator=(const PhSensor &) = delete;

private:
    gpio_num_t po_gpio_;
    gpio_num_t to_gpio_;
    gpio_num_t do_gpio_;

    bool initialized_;
    adc_oneshot_unit_handle_t adc_handle_;
    adc_cali_handle_t cali_handle_;

    bool configureChannels();
    bool cleanupCalibration();
    bool readAdcVoltageMv(adc_channel_t channel, int *voltage_mv);
    static float convertVoltageToPh(int voltage_mv);
    static float convertVoltageToTemp(int voltage_mv);
};

#endif // PH_SENSOR_H_
