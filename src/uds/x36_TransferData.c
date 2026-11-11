#include "handlers.h"
#include "boot_state.h"
#include "../flash/nvm.h"
#include "services/iso14229-1/36_transferData.h"

#include <string.h>

UDS_NRC_t x36_TransferData(void *context, const void *request,
                           void *response, void *user) {
    (void)context;
    (void)response;
    (void)user;

    const UDS_36_TD_Request *transfer = request;

    if (!boot_state.authenticated) {
        return UDS_NRC_SECURITY_ACCESS_DENIED;
    }
    if (!boot_state.transfer_active) {
        return UDS_NRC_REQUEST_SEQUENCE_ERROR;
    }

    size_t length = transfer->transferRequestParameterRecordLen;
    if (!length || length > BOOT_MAX_TRANSFER_DATA || length % 16) {
        return UDS_NRC_IMLOIF;
    }

    if (transfer->blockSequenceCounter == (uint8_t)(boot_state.next_sequence - 1) &&
        boot_state.last_transfer_length == length &&
        boot_state.transferred >= length &&
        !memcmp((const void *)(boot_state.transfer_start + boot_state.transferred - length),
                transfer->transferRequestParameterRecord, length)) {
        return UDS_NRC_PR;
    }

    if (transfer->blockSequenceCounter != boot_state.next_sequence) {
        return UDS_NRC_WRONG_BLOCK_SEQUENCE_NUMBER;
    }
    if (length > boot_state.transfer_size - boot_state.transferred) {
        return UDS_NRC_REQUEST_OUT_OF_RANGE;
    }

    if (!flash_write_app(boot_state.transfer_start + boot_state.transferred,
                         transfer->transferRequestParameterRecord, length)) {
        boot_state.transfer_active = false;
        return UDS_NRC_GENERAL_PROGRAMMING_FAILURE;
    }

    boot_state.transferred += (uint32_t)length;
    boot_state.last_transfer_length = (uint16_t)length;
    ++boot_state.next_sequence;
    return UDS_NRC_PR;
}
