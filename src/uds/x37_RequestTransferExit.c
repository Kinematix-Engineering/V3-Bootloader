#include "handlers.h"
#include "boot_state.h"
#include "services/iso14229-1/37_requestTransferExit.h"

UDS_NRC_t x37_RequestTransferExit(void *context, const void *request,
                                  void *response, void *user) {
    (void)context;
    (void)response;
    (void)user;

    const UDS_37_RTE_Request *exit_request = request;

    if (!boot_state.authenticated) {
        return UDS_NRC_SECURITY_ACCESS_DENIED;
    }
    if (!boot_state.transfer_active) {
        return UDS_NRC_REQUEST_SEQUENCE_ERROR;
    }
    if (exit_request->transferRequestParameterRecordLen) {
        return UDS_NRC_IMLOIF;
    }
    if (boot_state.transferred != boot_state.transfer_size) {
        return UDS_NRC_REQUEST_SEQUENCE_ERROR;
    }

    boot_state.transfer_active = false;
    return UDS_NRC_PR;
}
