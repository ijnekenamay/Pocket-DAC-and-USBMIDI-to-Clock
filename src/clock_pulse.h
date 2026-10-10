#pragma once

#include <cstdint>

namespace picointerface {

class ClockPulse {
public:
    static constexpr std::uint32_t pulse_width_us = 2'000;
    static constexpr std::uint32_t max_tempo_bpm = 560;
    static constexpr std::uint32_t midi_clock_pulses_per_quarter = 24;
    static constexpr std::uint32_t minimum_interval_us =
        60'000'000u / (max_tempo_bpm * midi_clock_pulses_per_quarter);

    static_assert(pulse_width_us < minimum_interval_us);

    void trigger(std::uint32_t now_us) noexcept {
        active_ = true;
        deadline_us_ = now_us + pulse_width_us;
    }

    bool update(std::uint32_t now_us) noexcept {
        if (active_ && static_cast<std::uint32_t>(now_us - deadline_us_) < 0x80000000u) {
            active_ = false;
        }
        return active_;
    }

private:
    std::uint32_t deadline_us_ = 0;
    bool active_ = false;
};

}