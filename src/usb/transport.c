#include "transport.h"
#include "device.h"
#include "protocol/protocol.h"
#include "../uds/boot_services.h"

enum {
    MAX_UDS_MESSAGE = KMX_PROTOCOL_MAX_PAYLOAD,
    MAX_COBS_FRAME = KMX_PROTOCOL_MAX_FRAME,
};

static uint8_t frame[MAX_COBS_FRAME];
static size_t frame_len;
static bool discard;
static bool boot_session_response_pending;
static bool reset_ready;

void usb_transport_init(bool pending) {
    frame_len = 0;
    discard = false;
    boot_session_response_pending = pending;
    reset_ready = false;
}

void usb_transport_reset(void) {
    frame_len = 0;
    discard = false;
    reset_ready = false;
    boot_uds_disconnect();
}

bool usb_transport_take_reset(void) {
    bool ready = reset_ready;
    reset_ready = false;
    return ready;
}

bool usb_transport_send(kmx_message_type_t type,
                        const uint8_t *payload, size_t payload_len) {
    uint8_t encoded[KMX_PROTOCOL_MAX_FRAME];
    size_t length = kmx_protocol_encode(type, payload, payload_len,
                                        encoded, sizeof(encoded));
    return length && usb_device_write(encoded, length);
}

void usb_transport_poll(void) {
    if (reset_ready) {
        return;
    }
    if (!usb_device_poll()) {
        usb_transport_reset();
        return;
    }

    if (boot_session_response_pending && usb_device_ready()) {
        static const uint8_t session_response[] = {0x50, 0x02};
        if (usb_transport_send(KMX_MESSAGE_UDS_DIAGNOSTIC,
                               session_response, sizeof(session_response))) {
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
            uint8_t request[KMX_PROTOCOL_MAX_DECODED];
            uint8_t response[MAX_UDS_MESSAGE];
            kmx_message_t message;

            if (kmx_protocol_decode(frame, frame_len, request, sizeof(request),
                                    &message) == KMX_PROTOCOL_OK &&
                message.type == KMX_MESSAGE_UDS_DIAGNOSTIC) {
                size_t response_len = boot_uds_handle(message.payload, message.payload_len,
                                                      response, sizeof(response));
                bool reset_requested = boot_uds_take_reset() != 0;
                if (response_len) {
                    bool sent = usb_transport_send(KMX_MESSAGE_UDS_DIAGNOSTIC,
                                                   response, response_len);
                    if (sent && reset_requested && response_len == 2 &&
                        response[0] == 0x51 && response[1] == 0x01) {
                        reset_ready = true;
                        frame_len = 0;
                        discard = false;
                        return;
                    }
                }
            }
        }

        frame_len = 0;
        discard = false;
    }
}
