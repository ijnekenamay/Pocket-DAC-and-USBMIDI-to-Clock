#pragma once

#include "audio_ring_buffer.h"

namespace picointerface {

void i2s_output_initialize() noexcept;
void i2s_output_service(AudioRingBuffer& audio_buffer) noexcept;
std::uint32_t i2s_output_sample_rate_hz() noexcept;

}