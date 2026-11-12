#pragma once

#include <stdbool.h>
#include "protocol/protocol.h"

/** Arms the one-shot session response for an ASW-requested boot. */
void usb_transport_init(bool boot_session_response_pending);

/** Receives COBS frames, dispatches UDS, and sends framed responses. */
void usb_transport_poll(void);

/** Consumes a reset only after its positive response finished transmitting. */
bool usb_transport_take_reset(void);

/** Clears a partial USB frame and the connection's diagnostic state. */
void usb_transport_reset(void);

/** Sends one complete typed frame. Call from the transport's polling context;
 *  this interface is not reentrant or interrupt safe. Returns false on failure. */
bool usb_transport_send(kmx_message_type_t type,
                        const uint8_t *payload, size_t payload_len);
