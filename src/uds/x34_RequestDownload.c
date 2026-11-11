#include "handlers.h"
#include "boot_state.h"
#include "../flash/nvm.h"
#include "services/iso14229-1/34_requestDownload.h"

static const uint8_t max_block_length[2] = {0x01, 0x02};

static bool range_erased(uint32_t address, uint32_t length) {
    uint32_t first = address / BOOT_ERASE_BLOCK_SIZE;
    uint32_t last = (address + length - 1) / BOOT_ERASE_BLOCK_SIZE;

    for (uint32_t block = first; block <= last; ++block) {
        if (!(boot_state.erased[block / 8] & (1u << (block % 8)))) {
            return false;
        }
    }

    return true;
}

UDS_NRC_t x34_RequestDownload(void *context, const void *request,
                              void *response, void *user) {
    (void)context;
    (void)user;

    const UDS_34_RD_Request *download = request;
    UDS_34_RD_Response *result = response;

    if (!boot_state.authenticated) {
        return UDS_NRC_SECURITY_ACCESS_DENIED;
    }
    if (boot_state.transfer_active) {
        return UDS_NRC_REQUEST_SEQUENCE_ERROR;
    }
    if (download->dataFormatIdentifier || download->memoryAddressLen != 4 ||
        download->memorySizeLen != 4) {
        return UDS_NRC_REQUEST_OUT_OF_RANGE;
    }

    uint32_t address = boot_be32(download->memoryAddress);
    uint32_t length = boot_be32(download->memorySize);

    if (!flash_app_range(address, length) || address % 16 || length % 16 ||
        !range_erased(address, length)) {
        return UDS_NRC_REQUEST_OUT_OF_RANGE;
    }

    boot_state.transfer_start = address;
    boot_state.transfer_size = length;
    boot_state.transferred = 0;
    boot_state.last_transfer_length = 0;
    boot_state.next_sequence = 1;
    boot_state.transfer_active = true;

    result->maxNumberOfBlockLength = max_block_length;
    result->maxNumberOfBlockLengthLen = sizeof(max_block_length);
    return UDS_NRC_PR;
}
