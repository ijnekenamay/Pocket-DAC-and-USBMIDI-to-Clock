#include "../src/clock_pulse.h"
#include "../src/audio_ring_buffer.h"
#include "../src/midi_clock.h"
#include "../src/usb_frame_count.h"

#include <cassert>
#include <cstdint>

int main() {
    picointerface::ClockPulse pulse;

    pulse.trigger(1'000);
    assert(pulse.update(2'999));
    assert(!pulse.update(3'000));

    pulse.trigger(10'000);
    pulse.trigger(11'000);
    assert(pulse.update(12'999));
    assert(!pulse.update(13'000));

    pulse.trigger(0xFFFFFF00u);
    assert(pulse.update(0x000006CFu));
    assert(!pulse.update(0x000006D0u));

    const std::uint8_t clock_packet[4] = {0x0f, 0xf8, 0x00, 0x00};
    const std::uint8_t note_packet[4] = {0x09, 0x90, 0x3c, 0x7f};
    const std::uint8_t other_realtime_packet[4] = {0x0f, 0xfa, 0x00, 0x00};
    assert(picointerface::is_midi_clock_packet(clock_packet));
    assert(!picointerface::is_midi_clock_packet(note_packet));
    assert(!picointerface::is_midi_clock_packet(other_realtime_packet));
    assert(picointerface::elapsed_usb_frames(100, 228) == 128);
    assert(picointerface::elapsed_usb_frames(2040, 120) == 128);
    assert(picointerface::elapsed_usb_frames(2040, 0) == 8);

    picointerface::AudioRingBuffer audio_buffer;
    picointerface::AudioFrame frame{};
    assert(!audio_buffer.pop(frame));
    assert(audio_buffer.push({123, -456}));
    assert(audio_buffer.pop(frame));
    assert(frame.left == 123);
    assert(frame.right == -456);
    for (std::size_t frame_index = 0;
         frame_index < picointerface::AudioRingBuffer::capacity - 1;
         ++frame_index) {
        assert(audio_buffer.push({static_cast<std::int32_t>(frame_index), 0}));
    }
    assert(!audio_buffer.push({0, 0}));
    assert(audio_buffer.dropped_frames() == 1);
    audio_buffer.clear();
    assert(!audio_buffer.pop(frame));
}