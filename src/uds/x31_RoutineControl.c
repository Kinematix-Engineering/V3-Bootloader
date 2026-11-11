#include "handlers.h"
#include "KMX_UDS.h"
#include "boot_state.h"
#include "../flash/nvm.h"
#include "services/iso14229-1/31_routineControl.h"

UDS_NRC_t x31_RoutineControl(void *context, const void *request,
                             void *response, void *user) {
    (void)context;
    (void)response;
    (void)user;

    const UDS_31_RC_Request *routine = request;

    if (!boot_state.authenticated) {
        return UDS_NRC_SECURITY_ACCESS_DENIED;
    }
    if (routine->routineControlType.raw != 1) {
        return UDS_NRC_REQUEST_OUT_OF_RANGE;
    }

    if (routine->routineIdentifier == KMX_ROUTINE_BOOT_VALIDATE) {
        if (routine->routineControlOptionBufLen != 12) {
            return UDS_NRC_IMLOIF;
        }
        if (boot_state.transfer_active) {
            return UDS_NRC_REQUEST_SEQUENCE_ERROR;
        }

        const uint8_t *options = routine->routineControlOptionBuf;
        uint32_t length = boot_be32(options);
        if (length % 4 || !flash_app_range(APP_START, length)) {
            return UDS_NRC_REQUEST_OUT_OF_RANGE;
        }

        uint32_t actual_xor;
        uint32_t actual_sum;
        flash_app_checksums(length, &actual_xor, &actual_sum);

        if (actual_xor != boot_be32(options + 4) ||
            actual_sum != boot_be32(options + 8)) {
            return UDS_NRC_GENERAL_PROGRAMMING_FAILURE;
        }

        return UDS_NRC_PR;
    }

    if (routine->routineIdentifier != KMX_ROUTINE_BOOT_ERASE) {
        return UDS_NRC_REQUEST_OUT_OF_RANGE;
    }
    if (routine->routineControlOptionBufLen != 4) {
        return UDS_NRC_IMLOIF;
    }

    uint32_t address = boot_be32(routine->routineControlOptionBuf);
    if (!flash_app_range(address, BOOT_ERASE_BLOCK_SIZE) ||
        address % BOOT_ERASE_BLOCK_SIZE) {
        return UDS_NRC_REQUEST_OUT_OF_RANGE;
    }

    boot_state.transfer_active = false;
    if (!flash_erase_app(address, BOOT_ERASE_BLOCK_SIZE)) {
        return UDS_NRC_GENERAL_PROGRAMMING_FAILURE;
    }

    uint32_t block = address / BOOT_ERASE_BLOCK_SIZE;
    boot_state.erased[block / 8] |= (uint8_t)(1u << (block % 8));
    return UDS_NRC_PR;
}
