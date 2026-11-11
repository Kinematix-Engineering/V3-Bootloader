#pragma once

#include <stdbool.h>

/** Arms the one-shot session response for an ASW-requested boot. */
void usb_transport_init(bool boot_session_response_pending);

/** Receives COBS frames, dispatches UDS, and sends framed responses. */
void usb_transport_poll(void);

/** Clears a partial USB frame and the connection's diagnostic state. */
void usb_transport_reset(void);
