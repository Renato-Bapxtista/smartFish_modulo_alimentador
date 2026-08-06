#ifndef MQ135_H_
#define MQ135_H_

#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"

class Mq135Sensor {
public:
    explicit Mq135Sensor(gpio_num_t do_gpio = GPIO_NUM_33,  // Changed from 34 to 33 to avoid conflict
                         gpio_num_t ao_gpio = GPIO_NUM_35,  // ADC1_CH7
                         adc_channel_t channel = ADC_CHANNEL_7);
    ~Mq135Sensor();

    bool init(adc_oneshot_unit_handle_t adc_handle);
    bool readRaw(int *raw_value);
    bool readVoltageMv(int *voltage_mv);
    bool readPpm(float *ppm_value);
    bool readDigital(bool *state);

    Mq135Sensor(const Mq135Sensor &) = delete;
    Mq135Sensor &operator=(const Mq135Sensor &) = delete;

private:
    gpio_num_t do_gpio_;
    gpio_num_t ao_gpio_;
    adc_channel_t channel_;

    bool initialized_;
    adc_oneshot_unit_handle_t adc_handle_;
    adc_cali_handle_t cali_handle_;

    bool configureChannel();
    bool cleanupCalibration();
};

#endif // MQ135_H_
