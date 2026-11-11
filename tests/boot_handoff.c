#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "cobs.h"
#include "transport.h"

static bool configured;
static bool terminal_open;
static bool accept_write = true;
static unsigned writes;
static unsigned disconnects;
static uint8_t last_response[8];
static size_t last_response_len;
static uint8_t incoming[16];
static size_t incoming_len;
static unsigned requests;
static bool reset_requested;

bool usb_device_poll(void) {
    return configured;
}

bool usb_device_ready(void) {
    return configured && terminal_open;
}

size_t usb_device_read(uint8_t *data, size_t capacity) {
    assert(incoming_len <= capacity);
    size_t length = incoming_len;
    memcpy(data, incoming, length);
    incoming_len = 0;
    return length;
}

bool usb_device_write(const uint8_t *data, size_t length) {
    assert(!usb_transport_take_reset());
    ++writes;
    last_response_len = length;
    memcpy(last_response, data, length);
    return accept_write;
}

void boot_uds_disconnect(void) {
    ++disconnects;
    reset_requested = false;
}

int boot_uds_take_reset(void) {
    bool requested = reset_requested;
    reset_requested = false;
    return requested;
}

size_t boot_uds_handle(const uint8_t *request, size_t request_len,
                       uint8_t *response, size_t response_capacity) {
    assert(request_len == 2 && request[0] == 0x11 && request[1] == 1);
    assert(response_capacity >= 2);
    ++requests;
    reset_requested = true;
    response[0] = 0x51;
    response[1] = 1;
    return 2;
}

int main(void) {
    usb_transport_init(true);

    usb_transport_poll();
    assert(disconnects == 1 && writes == 0);

    configured = true;
    usb_transport_poll();
    assert(writes == 0);

    terminal_open = true;
    accept_write = false;
    usb_transport_poll();
    assert(writes == 1);

    accept_write = true;
    usb_transport_poll();
    assert(writes == 2);

    uint8_t decoded[8];
    size_t decoded_len = 0;
    assert(cobs_decode(last_response, last_response_len - 1,
                       decoded, sizeof(decoded), &decoded_len));
    assert(decoded_len == 2 && decoded[0] == 0x50 && decoded[1] == 0x02);

    usb_transport_poll();
    assert(writes == 2);

    configured = false;
    usb_transport_poll();
    assert(disconnects == 2);

    configured = true;
    usb_transport_poll();
    assert(writes == 2);

    usb_transport_init(false);
    usb_transport_poll();
    assert(writes == 2);

    const uint8_t reset[] = {0x11, 1};
    incoming_len = cobs_encode(reset, sizeof(reset), incoming, sizeof(incoming));
    accept_write = false;
    usb_transport_poll();
    assert(requests == 1 && !usb_transport_take_reset());

    /* A retry succeeds; a pipelined request must not run after reset is ready. */
    accept_write = true;
    size_t length = cobs_encode(reset, sizeof(reset), incoming, sizeof(incoming));
    memcpy(incoming + length, incoming, length);
    incoming_len = 2 * length;
    usb_transport_poll();
    assert(requests == 2);
    assert(cobs_decode(last_response, last_response_len - 1,
                       decoded, sizeof(decoded), &decoded_len));
    assert(decoded_len == 2 && decoded[0] == 0x51 && decoded[1] == 1);
    assert(usb_transport_take_reset());
    assert(!usb_transport_take_reset());
}
