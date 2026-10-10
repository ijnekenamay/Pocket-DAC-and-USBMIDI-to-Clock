#include "clock_output.h"
#include "i2s_output.h"
#include "midi_clock.h"
#include "usb_audio.h"

#include "tusb.h"

int main() {
    picointerface::ClockOutput clock_output;
    picointerface::MidiClockDivider clock_divider;
    clock_output.initialize();
    picointerface::i2s_output_initialize();

    tud_init(0);

    while (true) {
        tud_task();

        std::uint8_t packet[4];
        while (tud_midi_available()) {
            if (!tud_midi_packet_read(packet)) {
                break;
            }
            if (picointerface::is_midi_start_packet(packet)) {
                clock_divider.reset();
            } else if (picointerface::is_midi_clock_packet(packet) && clock_divider.advance()) {
                clock_output.trigger();
            }
        }

        clock_output.update();
        picointerface::i2s_output_service(picointerface::usb_audio_buffer());
    }
}