#pragma once

#include <stddef.h>
#include <stdint.h>

/** Calculates XOR and wrapping sum over little-endian 32-bit words. */
void flash_checksum_words(const uint8_t *data, size_t length,
                          uint32_t *xor32, uint32_t *sum32);
