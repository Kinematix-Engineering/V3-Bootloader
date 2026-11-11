#include <assert.h>
#include <stdint.h>

#include "checksum.h"

int main(void) {
    const uint8_t image[16] = {
        0x5a, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff,
    };
    uint32_t xor32;
    uint32_t sum32;

    flash_checksum_words(image, sizeof(image), &xor32, &sum32);
    assert(xor32 == 0x000000a5);
    assert(sum32 == 0xffffff57);
}
