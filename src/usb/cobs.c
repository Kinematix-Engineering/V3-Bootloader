#include "cobs.h"

size_t cobs_encode(const uint8_t *src, size_t len, uint8_t *dst, size_t capacity) {
    if (!src || !dst || capacity < 2) {
        return 0;
    }

    size_t out = 1, code_at = 0;
    uint8_t code = 1;

    for (size_t i = 0; i < len; ++i) {
        if (src[i] == 0) {
            dst[code_at] = code;
            code_at = out++;
            code = 1;
            if (out > capacity) {
                return 0;
            }
        } else {
            if (out >= capacity) {
                return 0;
            }

            dst[out++] = src[i];
            if (++code == 0xff) {
                dst[code_at] = code;
                code_at = out++;
                code = 1;
                if (out > capacity) {
                    return 0;
                }
            }
        }
    }

    if (out >= capacity) {
        return 0;
    }

    dst[code_at] = code;
    dst[out++] = 0;
    return out;
}

bool cobs_decode(const uint8_t *src, size_t len, uint8_t *dst,
                 size_t capacity, size_t *decoded_len) {
    if (!src || !dst || !decoded_len || !len) {
        return false;
    }

    size_t in = 0, out = 0;

    while (in < len) {
        uint8_t code = src[in++];
        if (!code || (size_t)(code - 1) > len - in) {
            return false;
        }

        for (uint8_t i = 1; i < code; ++i) {
            if (out >= capacity || src[in] == 0) {
                return false;
            }

            dst[out++] = src[in++];
        }

        if (code != 0xff && in < len) {
            if (out >= capacity) {
                return false;
            }

            dst[out++] = 0;
        }
    }

    *decoded_len = out;
    return true;
}
