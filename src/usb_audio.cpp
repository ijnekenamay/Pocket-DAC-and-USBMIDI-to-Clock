#include "usb_audio.h"

#include "audio_config.h"
#include "i2s_output.h"
#include "usb_frame_count.h"
#include "usb_descriptors.h"

#include "hardware/clocks.h"
#include "pico/time.h"
#include "tusb.h"

namespace {

picointerface::AudioRingBuffer audio_buffer;
constexpr std::size_t audio_packet_word_count =
    CFG_TUD_AUDIO_FUNC_1_EP_OUT_SZ_MAX / sizeof(std::uint32_t);
static_assert(CFG_TUD_AUDIO_FUNC_1_EP_OUT_SZ_MAX % sizeof(std::uint32_t) == 0);
std::uint32_t audio_packet[audio_packet_word_count];
volatile bool feedback_measurement_started = false;
volatile std::uint32_t feedback_start_time_us = 0;
volatile std::uint32_t feedback_start_frame = 0;
volatile std::uint32_t feedback_master_clock_hz = 0;
constexpr std::uint32_t feedback_measurement_frames = 128;

}

namespace picointerface {

AudioRingBuffer& usb_audio_buffer() noexcept {
    return audio_buffer;
}

}

extern "C" bool tud_audio_get_req_entity_cb(
    std::uint8_t rhport,
    tusb_control_request_t const* control_request) {
    const auto* request = reinterpret_cast<audio_control_request_t const*>(control_request);
    if (request->bEntityID != UAC2_ENTITY_CLOCK) {
        return false;
    }

    if (request->bControlSelector == AUDIO_CS_CTRL_SAM_FREQ) {
        if (request->bRequest == AUDIO_CS_REQ_CUR) {
            audio_control_cur_4_t current{};
            current.bCur = tu_htole32(PICOINTERFACE_AUDIO_SAMPLE_RATE_HZ);
            return tud_audio_buffer_and_schedule_control_xfer(
                rhport, control_request, &current, sizeof(current));
        }
        if (request->bRequest == AUDIO_CS_REQ_RANGE) {
            audio_control_range_4_n_t(1) range{};
            range.wNumSubRanges = tu_htole16(1);
            range.subrange[0].bMin = PICOINTERFACE_AUDIO_SAMPLE_RATE_HZ;
            range.subrange[0].bMax = PICOINTERFACE_AUDIO_SAMPLE_RATE_HZ;
            range.subrange[0].bRes = 0;
            return tud_audio_buffer_and_schedule_control_xfer(
                rhport, control_request, &range, sizeof(range));
        }
    }

    if (request->bControlSelector == AUDIO_CS_CTRL_CLK_VALID &&
        request->bRequest == AUDIO_CS_REQ_CUR) {
        audio_control_cur_1_t valid{};
        valid.bCur = 1;
        return tud_audio_buffer_and_schedule_control_xfer(
            rhport, control_request, &valid, sizeof(valid));
    }

    return false;
}

extern "C" bool tud_audio_set_req_entity_cb(
    std::uint8_t rhport,
    tusb_control_request_t const* control_request,
    std::uint8_t* buffer) {
    (void)rhport;
    const auto* request = reinterpret_cast<audio_control_request_t const*>(control_request);
    if (request->bEntityID != UAC2_ENTITY_CLOCK ||
        request->bControlSelector != AUDIO_CS_CTRL_SAM_FREQ ||
        request->bRequest != AUDIO_CS_REQ_CUR ||
        request->wLength != sizeof(audio_control_cur_4_t)) {
        return false;
    }

    const auto* requested_rate = reinterpret_cast<audio_control_cur_4_t const*>(buffer);
    return tu_le32toh(requested_rate->bCur) == PICOINTERFACE_AUDIO_SAMPLE_RATE_HZ;
}

extern "C" bool tud_audio_set_itf_cb(
    std::uint8_t rhport,
    tusb_control_request_t const* request) {
    (void)rhport;
    const std::uint8_t interface_number = tu_u16_low(tu_le16toh(request->wIndex));
    const std::uint8_t alternate_setting = tu_u16_low(tu_le16toh(request->wValue));
    if (interface_number == ITF_NUM_AUDIO_STREAMING) {
        audio_buffer.clear();
        feedback_measurement_started = false;
        if (alternate_setting != 0) {
            const std::uint64_t initial_feedback =
                (static_cast<std::uint64_t>(picointerface::i2s_output_sample_rate_hz()) << 16) / 1000u;
            tud_audio_fb_set(static_cast<std::uint32_t>(initial_feedback));
        }
    }
    return true;
}

extern "C" void tud_audio_feedback_params_cb(
    std::uint8_t function_id,
    std::uint8_t alternate_interface,
    audio_feedback_params_t* parameters) {
    (void)function_id;
    (void)alternate_interface;
    parameters->method = AUDIO_FEEDBACK_METHOD_FREQUENCY_FIXED;
    parameters->sample_freq = picointerface::i2s_output_sample_rate_hz();
    parameters->frequency.mclk_freq = clock_get_hz(clk_sys);
    feedback_master_clock_hz = parameters->frequency.mclk_freq;
}

extern "C" TU_ATTR_FAST_FUNC void tud_audio_feedback_interval_isr(
    std::uint8_t function_id,
    std::uint32_t frame_number,
    std::uint8_t interval_shift) {
    (void)interval_shift;
    const std::uint32_t now_us = time_us_32();
    if (!feedback_measurement_started) {
        feedback_start_time_us = now_us;
        feedback_start_frame = frame_number;
        feedback_measurement_started = true;
        return;
    }

    const std::uint32_t measured_frames =
        picointerface::elapsed_usb_frames(feedback_start_frame, frame_number);
    if (measured_frames < feedback_measurement_frames) {
        return;
    }

    const std::uint32_t elapsed_us = now_us - feedback_start_time_us;
    if (elapsed_us == 0 || feedback_master_clock_hz == 0) {
        return;
    }

    const std::uint64_t elapsed_cycles =
        static_cast<std::uint64_t>(elapsed_us) * feedback_master_clock_hz;
    const std::uint64_t denominator = 1'000'000u * static_cast<std::uint64_t>(measured_frames);
    const std::uint32_t cycles_per_frame = static_cast<std::uint32_t>(
        (elapsed_cycles + denominator / 2u) / denominator);
    tud_audio_feedback_update(function_id, cycles_per_frame);

    feedback_start_time_us = now_us;
    feedback_start_frame = frame_number;
}

extern "C" bool tud_audio_rx_done_pre_read_cb(
    std::uint8_t rhport,
    std::uint16_t bytes_received,
    std::uint8_t function_id,
    std::uint8_t endpoint_out,
    std::uint8_t alternate_setting) {
    (void)rhport;
    (void)function_id;
    (void)endpoint_out;
    (void)alternate_setting;

    const std::uint16_t bytes_read = tud_audio_read(
        reinterpret_cast<std::uint8_t*>(audio_packet), bytes_received);
    constexpr std::size_t bytes_per_frame =
        PICOINTERFACE_AUDIO_CHANNEL_COUNT * PICOINTERFACE_AUDIO_SUBSLOT_BYTES;
    const std::size_t frame_count = bytes_read / bytes_per_frame;
    for (std::size_t frame_index = 0; frame_index < frame_count; ++frame_index) {
        const std::size_t sample_index = frame_index * PICOINTERFACE_AUDIO_CHANNEL_COUNT;
        audio_buffer.push({
            static_cast<std::int32_t>(audio_packet[sample_index]),
            static_cast<std::int32_t>(audio_packet[sample_index + 1])
        });
    }
    return true;
}