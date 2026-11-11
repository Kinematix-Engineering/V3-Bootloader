#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** Checks that a nonempty range lies within application space in Bank A. */
bool flash_app_range(uint32_t address, uint32_t length);

/** Erases aligned application flash blocks. */
bool flash_erase_app(uint32_t address, uint32_t length);

/** Programs aligned application quad words and verifies their contents. */
bool flash_write_app(uint32_t address, const uint8_t *data, size_t length);

/** Calculates checksums over application flash starting at APP_START. */
void flash_app_checksums(uint32_t length, uint32_t *xor32, uint32_t *sum32);
