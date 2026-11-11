#include "transport.h"
#include "device.h"
#include "cobs.h"
#include "../uds/boot_services.h"

enum {
    MAX_UDS_MESSAGE = 512,
    MAX_COBS_FRAME = 516,
};

static uint8_t frame[MAX_COBS_FRAME];
static size_t frame_len;
static bool discard;
static bool boot_session_response_pending;

void usb_transport_init(bool pending) {
    frame_len = 0;
    discard = false;
    boot_session_response_pending = pending;
}

void usb_transport_reset(void) {
    frame_len = 0;
    discard = false;
    boot_uds_disconnect();
}

void usb_transport_poll(void) {
    if (!usb_device_poll()) {
        usb_transport_reset();
        return;
    }

    if (boot_session_response_pending && usb_device_ready()) {
        static const uint8_t session_response[] = {0x50, 0x02};
        uint8_t encoded[4];
        size_t length = cobs_encode(session_response, sizeof(session_response),
                                    encoded, sizeof(encoded));

        if (length && usb_device_write(encoded, length)) {
            boot_session_response_pending = false;
        }
    }

    uint8_t bytes[64];
    size_t received = usb_device_read(bytes, sizeof(bytes));

    for (size_t i = 0; i < received; ++i) {
        if (bytes[i]) {
            if (!discard && frame_len < sizeof(frame)) {
                frame[frame_len++] = bytes[i];
            } else {
                discard = true;
            }
            continue;
        }

        if (!discard && frame_len) {
            uint8_t request[MAX_UDS_MESSAGE];
            uint8_t response[MAX_UDS_MESSAGE];
            uint8_t encoded[MAX_COBS_FRAME];
            size_t request_len = 0;

            if (cobs_decode(frame, frame_len, request, sizeof(request), &request_len)) {
                size_t response_len = boot_uds_handle(request, request_len,
                                                      response, sizeof(response));
                if (response_len) {
                    size_t encoded_len = cobs_encode(response, response_len,
                                                     encoded, sizeof(encoded));
                    if (encoded_len) {
                        usb_device_write(encoded, encoded_len);
                    }
                }
            }
        }

        frame_len = 0;
        discard = false;
    }
}
