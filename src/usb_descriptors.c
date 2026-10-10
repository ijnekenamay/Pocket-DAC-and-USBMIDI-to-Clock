#include "tusb.h"
#include "usb_descriptors.h"

#include <string.h>

#define USB_VID 0xcafe
#define USB_PID 0x4018
#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_AUDIO_SPEAKER_STEREO_DESC_LEN + TUD_MIDI_DESC_LEN)

static tusb_desc_device_t const device_descriptor = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = USB_VID,
    .idProduct = USB_PID,
    .bcdDevice = 0x0100,
    .iManufacturer = 0x01,
    .iProduct = 0x02,
    .iSerialNumber = 0x03,
    .bNumConfigurations = 0x01
};

static uint8_t const configuration_descriptor[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),
    TUD_AUDIO_SPEAKER_STEREO_DESCRIPTOR(2, 0x01, 0x81),
    TUD_MIDI_DESCRIPTOR(ITF_NUM_MIDI_CONTROL, 0, 0x02, 0x82, 64)
};

_Static_assert(sizeof(configuration_descriptor) == CONFIG_TOTAL_LEN,
    "USB configuration descriptor length does not match its contents");

uint8_t const* tud_descriptor_device_cb(void) {
    return (uint8_t const*)&device_descriptor;
}

uint8_t const* tud_descriptor_configuration_cb(uint8_t index) {
    (void)index;
    return configuration_descriptor;
}

uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t language_id) {
    (void)language_id;

    static uint8_t const language_id_descriptor[] = {0x09, 0x04};
    static char const* const strings[] = {
        NULL,
        "Pocket DAC",
        "Pocket DAC and USBMIDI to Clock",
        "PICO0001",
        "PocketGroove Audio"
    };
    static uint16_t descriptor[40];

    if (index >= sizeof(strings) / sizeof(strings[0])) {
        return NULL;
    }

    uint8_t character_count;
    if (index == 0) {
        memcpy(&descriptor[1], language_id_descriptor, sizeof(language_id_descriptor));
        character_count = 1;
    } else {
        character_count = (uint8_t)strlen(strings[index]);
        if (character_count > 39) {
            character_count = 39;
        }
        for (uint8_t character_index = 0; character_index < character_count; ++character_index) {
            descriptor[1 + character_index] = (uint8_t)strings[index][character_index];
        }
    }

    descriptor[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * character_count + 2));
    return descriptor;
}