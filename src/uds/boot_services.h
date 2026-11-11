#pragma once
#include <stddef.h>
#include <stdint.h>

/** Initializes the bootloader's UDS service table and connection state. */
void boot_uds_init(void);

/** Clears authentication and any active download after USB disconnect. */
void boot_uds_disconnect(void);

/** Processes one decoded UDS request into a response buffer. */
size_t boot_uds_handle(const uint8_t *request, size_t request_len,
                       uint8_t *response, size_t response_capacity);

/** Returns and clears a pending reset request. */
int boot_uds_take_reset(void);
