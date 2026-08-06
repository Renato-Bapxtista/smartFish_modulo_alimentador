#ifndef DS18B20_H_
#define DS18B20_H_

#include <cstddef>
#include <cstdint>
#include "driver/gpio.h"

class DS18B20 {
public:
    explicit DS18B20(gpio_num_t gpio = GPIO_NUM_4);

    bool init();
    bool readTemperature(float *temperature);

private:
    gpio_num_t gpio_;

    void driveLow();
    void release();
    bool reset();
    void writeBit(bool bit);
    bool readBit();
    void writeByte(uint8_t value);
    uint8_t readByte();
    uint8_t crc8(const uint8_t *data, size_t len);
    bool startConversion();
};

#endif // DS18B20_H_
