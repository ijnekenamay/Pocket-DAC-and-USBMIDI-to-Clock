#pragma once

#include <cstdint>

namespace picointerface {

constexpr std::uint32_t midi_clock_ticks_per_quarter = 24;
constexpr std::uint32_t output_clock_pulses_per_quarter = 2;
constexpr std::uint32_t midi_clock_ticks_per_output_pulse =
    midi_clock_ticks_per_quarter / output_clock_pulses_per_quarter;

static_assert(midi_clock_ticks_per_quarter % output_clock_pulses_per_quarter == 0);

constexpr bool is_midi_clock_packet(const std::uint8_t* packet) noexcept {
    return (packet[0] & 0x0fu) == 0x0fu && packet[1] == 0xf8u;
}

class MidiClockDivider {
public:
    bool advance() noexcept {
        ++ticks_;
        if (ticks_ < midi_clock_ticks_per_output_pulse) {
            return false;
        }

        ticks_ = 0;
        return true;
    }

    void reset() noexcept {
        ticks_ = 0;
    }

private:
    std::uint32_t ticks_ = 0;
};

}