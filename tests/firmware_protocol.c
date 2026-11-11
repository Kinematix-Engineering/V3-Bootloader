#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "boot_services.h"
#include "aes128.h"
#include "cobs.h"
#include "v3_boot_table.h"
#include "nvm.h"
#include "sam.h"

TestMclk test_mclk;
TestTrng test_trng = {.INTFLAG.bit.DATARDY = 1, .DATA.reg = 0x44332211};
static const V3BootTable test_table = {
    .boot_version = "v3bl-1.2.0",
};

const V3BootTable *v3_boot_table_get(void) {
    return &test_table;
}

static uint8_t serial[16];
static int erases;
static int writes;
static int validations;

uint32_t platform_millis(void) {
    return 0;
}

void platform_serial(uint8_t out[16]) {
    memcpy(out, serial, 16);
}

bool flash_erase_app(uint32_t address, uint32_t length) {
    assert(address == 0x4000 && length == 8192);
    ++erases;
    return true;
}

bool flash_write_app(uint32_t address, const uint8_t *data, size_t length) {
    assert(address == 0x4000 && length == 16 && data[0] == 0x5a);
    ++writes;
    return true;
}

void flash_app_checksums(uint32_t length, uint32_t *xor32, uint32_t *sum32) {
    assert(length == 16);
    ++validations;
    *xor32 = 0x000000a5;
    *sum32 = 0xffffff57;
}

static void expect(const uint8_t *request, size_t request_len,
                   const uint8_t *expected, size_t expected_len) {
    uint8_t response[512];
    size_t length = boot_uds_handle(request, request_len, response, sizeof(response));

    if (length != expected_len || memcmp(response, expected, length)) {
        fprintf(stderr, "Unexpected response to service 0x%02x\n", request[0]);
        assert(0);
    }
}

int main(void) {
    const uint8_t key[16] = {
        0x71, 0xc9, 0x2e, 0xa4, 0x58, 0x0b, 0xd7, 0x36,
        0xf1, 0x83, 0x4d, 0x6a, 0x9c, 0x25, 0xe0, 0x5b,
    };
    const uint8_t nonce[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    const uint8_t cipher[16] = {
        0xc6, 0xf8, 0xdc, 0xe6, 0xa5, 0x44, 0x9c, 0xff,
        0x3e, 0x23, 0x83, 0x47, 0x0c, 0xa9, 0x09, 0xc9,
    };

    uint8_t encrypted[16];
    aes128_encrypt(key, nonce, encrypted);
    assert(memcmp(encrypted, cipher, 16) == 0);

    uint8_t raw[512], encoded[516], decoded[512];
    for (size_t count = 0; count <= sizeof(raw); ++count) {
        for (unsigned mode = 0; mode < 2; ++mode) {
            for (size_t i = 0; i < count; ++i) {
                raw[i] = mode ? (uint8_t)(i % 253 + 1) : (uint8_t)(i % 17 ? i : 0);
            }

            size_t frame_len = cobs_encode(raw, count, encoded, sizeof(encoded));
            assert(frame_len && encoded[frame_len - 1] == 0);

            size_t decoded_len = 0;
            assert(cobs_decode(encoded, frame_len - 1, decoded, sizeof(decoded), &decoded_len));
            assert(decoded_len == count && memcmp(raw, decoded, count) == 0);
        }
    }

    for (unsigned i = 0; i < 16; ++i) {
        serial[i] = (uint8_t)i;
    }

    boot_uds_init();

    assert(!flash_app_range(0x3fff, 1));
    assert(flash_app_range(0x4000, 1));
    assert(flash_app_range(0x7ffff, 1));
    assert(!flash_app_range(0x7ffff, 2));
    assert(!flash_app_range(0x80000, 1));
    assert(!flash_app_range(0x4000, 0));
    assert(!flash_app_range(0xfffffff0, 0x40));

    const uint8_t session_request[] = {0x22, 0xf1, 0x86};
    const uint8_t session_response[] = {0x62, 0xf1, 0x86, 0x02};
    expect(session_request, sizeof(session_request), session_response, sizeof(session_response));
    const uint8_t serial_request[] = {0x22, 0xf1, 0x8c};
    uint8_t serial_response[19] = {0x62, 0xf1, 0x8c};
    memcpy(serial_response + 3, serial, 16);
    expect(serial_request, sizeof(serial_request), serial_response, sizeof(serial_response));
    const uint8_t read_request[] = {0x35};
    const uint8_t read_rejected[] = {0x7f, 0x35, 0x11};
    expect(read_request, sizeof(read_request), read_rejected, sizeof(read_rejected));
    const uint8_t erase_request[] = {0x31, 0x01, 0xff, 0x00, 0, 0, 0x40, 0};
    const uint8_t denied[] = {0x7f, 0x31, 0x33};
    expect(erase_request, sizeof(erase_request), denied, sizeof(denied));
    const uint8_t validate_request[] = {
        0x31, 0x01, 0xff, 0x01,
        0x00, 0x00, 0x00, 0x10,
        0x00, 0x00, 0x00, 0xa5,
        0xff, 0xff, 0xff, 0x57,
    };
    expect(validate_request, sizeof(validate_request), denied, sizeof(denied));

    const uint8_t seed_request[] = {0x27, 0x01};
    uint8_t seed_response[512];
    size_t length = boot_uds_handle(seed_request, sizeof(seed_request),
                                    seed_response, sizeof(seed_response));
    assert(length == 19 && seed_response[0] == 0x67 &&
           seed_response[1] == 1 && seed_response[2] == 1);

    uint8_t answer[18] = {0x27, 0x02};
    aes128_encrypt(key, seed_response + 3, answer + 2);

    for (unsigned i = 0; i < 16; ++i) {
        answer[i + 2] ^= serial[i];
    }

    const uint8_t unlocked[] = {0x67, 0x02};
    expect(answer, sizeof(answer), unlocked, sizeof(unlocked));

    const uint8_t bank_b_erase[] = {0x31, 0x01, 0xff, 0x00, 0x00, 0x08, 0x00, 0x00};
    const uint8_t out_of_range[] = {0x7f, 0x31, 0x31};
    expect(bank_b_erase, sizeof(bank_b_erase), out_of_range, sizeof(out_of_range));

    const uint8_t invalid_validate_length[] = {
        0x31, 0x01, 0xff, 0x01,
        0x00, 0x07, 0xc0, 0x01,
        0x00, 0x00, 0x00, 0xa5,
        0xff, 0xff, 0xff, 0x57,
    };
    expect(invalid_validate_length, sizeof(invalid_validate_length),
           out_of_range, sizeof(out_of_range));

    uint8_t zero_length_validate[sizeof(validate_request)];
    memcpy(zero_length_validate, validate_request, sizeof(zero_length_validate));
    memset(zero_length_validate + 4, 0, 4);
    expect(zero_length_validate, sizeof(zero_length_validate),
           out_of_range, sizeof(out_of_range));

    uint8_t beyond_bank_validate[sizeof(validate_request)];
    memcpy(beyond_bank_validate, validate_request, sizeof(beyond_bank_validate));
    beyond_bank_validate[5] = 0x07;
    beyond_bank_validate[6] = 0xc0;
    beyond_bank_validate[7] = 0x04;
    expect(beyond_bank_validate, sizeof(beyond_bank_validate),
           out_of_range, sizeof(out_of_range));

    const uint8_t short_validate[] = {0x31, 0x01, 0xff, 0x01};
    const uint8_t bad_format[] = {0x7f, 0x31, 0x13};
    expect(short_validate, sizeof(short_validate), bad_format, sizeof(bad_format));
    assert(validations == 0);

    const uint8_t crossing_download[] = {
        0x34, 0, 0x44, 0, 0x07, 0xff, 0xf0, 0, 0, 0, 0x20,
    };
    const uint8_t download_out_of_range[] = {0x7f, 0x34, 0x31};
    expect(crossing_download, sizeof(crossing_download),
           download_out_of_range, sizeof(download_out_of_range));
    expect(session_request, sizeof(session_request), session_response, sizeof(session_response));
    expect(read_request, sizeof(read_request), read_rejected, sizeof(read_rejected));

    const uint8_t erased[] = {0x71, 0x01, 0xff, 0x00, 0x00};
    expect(erase_request, sizeof(erase_request), erased, sizeof(erased));
    assert(erases == 1);

    const uint8_t download[] = {0x34, 0, 0x44, 0, 0, 0x40, 0, 0, 0, 0, 0x10};
    const uint8_t download_ok[] = {0x74, 0x20, 0x01, 0x02};
    expect(download, sizeof(download), download_ok, sizeof(download_ok));
    const uint8_t transfer_active[] = {0x7f, 0x31, 0x24};
    expect(validate_request, sizeof(validate_request),
           transfer_active, sizeof(transfer_active));
    uint8_t transfer[18] = {0x36, 1};
    memset(transfer + 2, 0x5a, 16);
    const uint8_t transfer_ok[] = {0x76, 1};
    expect(transfer, sizeof(transfer), transfer_ok, sizeof(transfer_ok));
    assert(writes == 1);

    const uint8_t exit_request[] = {0x37};
    const uint8_t exit_response[] = {0x77};
    expect(exit_request, sizeof(exit_request), exit_response, sizeof(exit_response));

    uint8_t wrong_checksum[sizeof(validate_request)];
    memcpy(wrong_checksum, validate_request, sizeof(wrong_checksum));
    wrong_checksum[15] ^= 1;
    const uint8_t checksum_failed[] = {0x7f, 0x31, 0x72};
    expect(wrong_checksum, sizeof(wrong_checksum),
           checksum_failed, sizeof(checksum_failed));

    wrong_checksum[15] ^= 1;
    wrong_checksum[11] ^= 1;
    expect(wrong_checksum, sizeof(wrong_checksum),
           checksum_failed, sizeof(checksum_failed));

    const uint8_t validated[] = {0x71, 0x01, 0xff, 0x01, 0x00};
    expect(validate_request, sizeof(validate_request), validated, sizeof(validated));
    assert(validations == 3);

    boot_uds_disconnect();
    expect(erase_request, sizeof(erase_request), denied, sizeof(denied));
    puts("firmware UDS protocol passed");
}
