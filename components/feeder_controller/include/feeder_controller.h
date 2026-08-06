#ifndef FEEDER_CONTROLLER_H
#define FEEDER_CONTROLLER_H

#include "driver/gpio.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define STEP_GPIO GPIO_NUM_26
#define DIR_GPIO  GPIO_NUM_25
#define EN_GPIO   GPIO_NUM_27

typedef struct {
    gpio_num_t step_pin;
    gpio_num_t dir_pin;
    gpio_num_t en_pin;
    uint32_t steps_per_gram;
    uint32_t step_delay_us;
} feeder_config_t;

void feeder_init(const feeder_config_t *config);
void feeder_toggle(void);
void feeder_stop(void);
bool feeder_is_running(void);

#ifdef __cplusplus
}
#endif

#endif // FEEDER_CONTROLLER_H
