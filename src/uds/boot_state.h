#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "sam.h"
#include "../hardware_addresses.h"

enum {
    BOOT_ERASE_BLOCK_SIZE = 8192,
    BOOT_MAX_TRANSFER_DATA = 256,
};

typedef struct {
    bool authenticated;
    bool seed_valid;
    uint8_t seed[17];
    uint8_t failures;
    uint32_t locked_until;
    uint8_t erased[BANK_A_END / BOOT_ERASE_BLOCK_SIZE / 8];
    bool transfer_active;
    uint32_t transfer_start;
    uint32_t transfer_size;
    uint32_t transferred;
    uint16_t last_transfer_length;
    uint8_t next_sequence;
    int reset_pending;
} BootState;

extern BootState boot_state;

/** Decodes a four-byte UDS address or length in network byte order. */
uint32_t boot_be32(const uint8_t data[4]);
