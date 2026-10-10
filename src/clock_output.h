#pragma once

#include "clock_pulse.h"

namespace picointerface {

class ClockOutput {
public:
    static constexpr unsigned int gpio_number = 15;

    void initialize() noexcept;
    void trigger() noexcept;
    void update() noexcept;

private:
    ClockPulse pulse_{};
    bool pulse_active_ = false;
};

}