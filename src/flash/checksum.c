#include "checksum.h"

void flash_checksum_words(const uint8_t *data, size_t length,
                          uint32_t *xor32, uint32_t *sum32) {
    uint32_t word_xor = 0;
    uint32_t word_sum = 0;

    for (size_t offset = 0; offset < length; offset += 4) {
        uint32_t word = (uint32_t)data[offset] |
                        ((uint32_t)data[offset + 1] << 8) |
                        ((uint32_t)data[offset + 2] << 16) |
                        ((uint32_t)data[offset + 3] << 24);

        word_xor ^= word;
        word_sum += word;
    }

    *xor32 = word_xor;
    *sum32 = word_sum;
}
