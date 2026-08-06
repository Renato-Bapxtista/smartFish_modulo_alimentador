#pragma once

#include "driver/gpio.h"

namespace button {

class Button {
public:
    enum class State {
        Released = 0,
        Pressed = 1
    };

    explicit Button(gpio_num_t pin, bool activeHigh = true);

    void begin();
    State read();
    bool isPressed() const;
    bool hasChanged() const;

private:
    gpio_num_t pin_;
    bool activeHigh_;
    State currentState_;
    bool changed_;
};

}  // namespace button
