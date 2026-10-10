#include "clock_output.h"

#include "hardware/gpio.h"
#include "pico/time.h"

namespace picointerface {

void ClockOutput::initialize() noexcept {
    gpio_init(gpio_number);
    gpio_put(gpio_number, true);
    gpio_set_dir(gpio_number, GPIO_OUT);
    pulse_active_ = false;
}

void ClockOutput::trigger() noexcept {
    pulse_.trigger(time_us_32());
    if (!pulse_active_) {
        gpio_put(gpio_number, false);
        pulse_active_ = true;
    }
}

void ClockOutput::update() noexcept {
    const bool is_active = pulse_.update(time_us_32());
    if (is_active != pulse_active_) {
        gpio_put(gpio_number, !is_active);
        pulse_active_ = is_active;
    }
}

}