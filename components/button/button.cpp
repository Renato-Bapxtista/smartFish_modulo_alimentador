#include "button.hpp"

#include "driver/gpio.h"

namespace button {

Button::Button(gpio_num_t pin, bool activeHigh)
    : pin_(pin), activeHigh_(activeHigh), currentState_(State::Released), changed_(false) {}

void Button::begin() {
    gpio_config_t io_conf;
    io_conf.pin_bit_mask = (1ULL << pin_);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;

    gpio_config(&io_conf);
}

Button::State Button::read() {
    int level = gpio_get_level(pin_);
    bool pressed = activeHigh_ ? (level == 1) : (level == 0);
    State nextState = pressed ? State::Pressed : State::Released;

    changed_ = (nextState != currentState_);
    currentState_ = nextState;
    return currentState_;
}

bool Button::isPressed() const {
    return currentState_ == State::Pressed;
}

bool Button::hasChanged() const {
    return changed_;
}

}  // namespace button
