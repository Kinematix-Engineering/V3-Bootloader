#include "handlers.h"
#include "boot_state.h"
#include "../crypto/aes128.h"
#include "../platform.h"
#include "services/iso14229-1/27_securityAccess.h"
#include "session_server/uds_server.h"

// Temporary version-1 access key shared with the host tool.
static const uint8_t access_key[16] = {
    0x71, 0xc9, 0x2e, 0xa4, 0x58, 0x0b, 0xd7, 0x36,
    0xf1, 0x83, 0x4d, 0x6a, 0x9c, 0x25, 0xe0, 0x5b,
};

static bool random_seed(uint8_t output[16]) {
    MCLK->APBCMASK.bit.TRNG_ = 1;
    TRNG->CTRLA.bit.ENABLE = 1;

    for (unsigned word = 0; word < 4; ++word) {
        uint32_t started = platform_millis();

        while (!TRNG->INTFLAG.bit.DATARDY) {
            if ((uint32_t)(platform_millis() - started) > 100u) {
                return false;
            }
        }

        uint32_t value = TRNG->DATA.reg;
        for (unsigned byte = 0; byte < 4; ++byte) {
            output[4 * word + byte] = (uint8_t)(value >> (8 * byte));
        }
    }

    return true;
}

UDS_NRC_t x27_SecurityAccess(void *context, const void *request,
                             void *response, void *user) {
    (void)user;

    const UDS_27_SA_Request *access = request;
    UDS_27_SA_Response *result = response;

    if (access->securityAccessType == 1) {
        if (access->securityAccessDataRecordLen) {
            return UDS_NRC_IMLOIF;
        }

        uint32_t now = platform_millis();
        if (boot_state.failures >= 5 &&
            (int32_t)(now - boot_state.locked_until) < 0) {
            return UDS_NRC_REQUIRED_TIME_DELAY_NOT_EXPIRED;
        }
        if ((int32_t)(now - boot_state.locked_until) >= 0) {
            boot_state.failures = 0;
        }

        boot_state.authenticated = false;
        boot_state.transfer_active = false;
        boot_state.seed_valid = false;
        boot_state.seed[0] = 1;

        if (!random_seed(&boot_state.seed[1])) {
            return UDS_NRC_CONDITIONS_NOT_CORRECT;
        }

        boot_state.seed_valid = true;
        result->securitySeed = boot_state.seed;
        result->securitySeedLen = sizeof(boot_state.seed);
        return UDS_NRC_PR;
    }

    if (access->securityAccessType != 2) {
        return UDS_NRC_SUBFUNCTION_NOT_SUPPORTED;
    }
    if (access->securityAccessDataRecordLen != 16) {
        return UDS_NRC_IMLOIF;
    }
    if (!boot_state.seed_valid) {
        return UDS_NRC_REQUEST_SEQUENCE_ERROR;
    }

    uint8_t encrypted[16];
    uint8_t serial[16];
    aes128_encrypt(access_key, &boot_state.seed[1], encrypted);
    platform_serial(serial);

    unsigned difference = 0;
    for (unsigned i = 0; i < 16; ++i) {
        difference |= (unsigned)(access->securityAccessDataRecord[i] ^
                                 (encrypted[i] ^ serial[i]));
    }

    boot_state.seed_valid = false;
    if (difference) {
        boot_state.authenticated = false;
        if (++boot_state.failures >= 5) {
            boot_state.locked_until = platform_millis() + 10000u;
        }
        return UDS_NRC_INVALID_KEY;
    }

    boot_state.failures = 0;
    boot_state.authenticated = true;
    ((UdsSession *)context)->security_level = 1;
    return UDS_NRC_PR;
}
