#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** Encodes one message and appends the zero frame delimiter. */
size_t cobs_encode(const uint8_t *src, size_t len, uint8_t *dst, size_t capacity);

/** Decodes one COBS frame without its zero delimiter. */
bool cobs_decode(const uint8_t *src, size_t len, uint8_t *dst,
                 size_t capacity, size_t *decoded_len);
