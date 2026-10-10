#pragma once

#include <cstdint>

namespace picointerface {

constexpr std::uint32_t usb_frame_number_mask = (1u << 11) - 1u;

constexpr std::uint32_t elapsed_usb_frames(
    std::uint32_t start_frame,
    std::uint32_t current_frame) noexcept {
    return (current_frame - start_frame) & usb_frame_number_mask;
}

}