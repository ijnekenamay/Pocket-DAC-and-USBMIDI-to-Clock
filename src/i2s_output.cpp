#include "i2s_output.h"

#include "audio_config.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "i2s_tx.pio.h"

namespace picointerface {
namespace {

PIO const i2s_pio = pio0;
constexpr std::uint32_t bclk_gpio = 10;
constexpr std::uint32_t data_gpio = 11;
constexpr std::uint32_t wsel_gpio = 12;
constexpr std::uint32_t nominal_sample_rate_hz = PICOINTERFACE_AUDIO_SAMPLE_RATE_HZ;
constexpr std::uint32_t pio_cycles_per_frame = 132;

uint i2s_state_machine;
std::uint32_t actual_sample_rate_hz = nominal_sample_rate_hz;

}

void i2s_output_initialize() noexcept {
    const uint program_offset = pio_add_program(i2s_pio, &i2s_tx_program);
    i2s_state_machine = pio_claim_unused_sm(i2s_pio, true);

    pio_sm_config config = i2s_tx_program_get_default_config(program_offset);
    sm_config_set_out_pins(&config, data_gpio, 1);
    sm_config_set_sideset_pins(&config, bclk_gpio);
    sm_config_set_set_pins(&config, wsel_gpio, 1);
    sm_config_set_out_shift(&config, false, true, PICOINTERFACE_AUDIO_SUBSLOT_BYTES * 8);
    sm_config_set_fifo_join(&config, PIO_FIFO_JOIN_TX);
    const std::uint32_t system_clock_hz = clock_get_hz(clk_sys);
    const float requested_divider = static_cast<float>(system_clock_hz) /
        (nominal_sample_rate_hz * pio_cycles_per_frame);
    std::uint16_t divider_integer;
    std::uint8_t divider_fraction;
    pio_calculate_clkdiv_from_float(requested_divider, &divider_integer, &divider_fraction);
    sm_config_set_clkdiv_int_frac(&config, divider_integer, divider_fraction);

    const std::uint32_t divider_fixed =
        static_cast<std::uint32_t>(divider_integer) * 256u + divider_fraction;
    const std::uint64_t rate_numerator = static_cast<std::uint64_t>(system_clock_hz) * 256u;
    const std::uint64_t rate_denominator =
        static_cast<std::uint64_t>(divider_fixed) * pio_cycles_per_frame;
    actual_sample_rate_hz = static_cast<std::uint32_t>(
        (rate_numerator + rate_denominator / 2u) / rate_denominator);

    pio_gpio_init(i2s_pio, bclk_gpio);
    pio_gpio_init(i2s_pio, data_gpio);
    pio_gpio_init(i2s_pio, wsel_gpio);
    pio_sm_set_consecutive_pindirs(i2s_pio, i2s_state_machine, bclk_gpio, 3, true);
    pio_sm_init(i2s_pio, i2s_state_machine, program_offset, &config);
    pio_sm_set_enabled(i2s_pio, i2s_state_machine, true);
}

void i2s_output_service(AudioRingBuffer& audio_buffer) noexcept {
    while (pio_sm_get_tx_fifo_level(i2s_pio, i2s_state_machine) <= 6) {
        AudioFrame frame{};
        audio_buffer.pop(frame);

        const std::uint32_t left_sample = static_cast<std::uint32_t>(frame.left >> 1);
        const std::uint32_t right_sample = static_cast<std::uint32_t>(frame.right >> 1);
        pio_sm_put(i2s_pio, i2s_state_machine, left_sample);
        pio_sm_put(i2s_pio, i2s_state_machine, right_sample);
    }
}

std::uint32_t i2s_output_sample_rate_hz() noexcept {
    return actual_sample_rate_hz;
}

}