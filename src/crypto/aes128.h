#pragma once

#include <stdint.h>

/** Encrypts one 16-byte block with a 16-byte AES key. */
void aes128_encrypt(const uint8_t key[16], const uint8_t input[16], uint8_t output[16]);
