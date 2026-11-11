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

bool usb_device_poll(void) {
    return configured;
}

bool usb_device_ready(void) {
    return configured && terminal_open;
}

size_t usb_device_read(uint8_t *data, size_t capacity) {
    (void)data;
    (void)capacity;
    return 0;
}

bool usb_device_write(const uint8_t *data, size_t length) {
    ++writes;
    last_response_len = length;
    memcpy(last_response, data, length);
    return accept_write;
}

void boot_uds_disconnect(void) {
    ++disconnects;
}

size_t boot_uds_handle(const uint8_t *request, size_t request_len,
                       uint8_t *response, size_t response_capacity) {
    (void)request;
    (void)request_len;
    (void)response;
    (void)response_capacity;
    return 0;
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
}
