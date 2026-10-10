#pragma once

#include <cstdint>

namespace picointerface {

constexpr bool is_midi_clock_packet(const std::uint8_t* packet) noexcept {
    return (packet[0] & 0x0fu) == 0x0fu && packet[1] == 0xf8u;
}

}