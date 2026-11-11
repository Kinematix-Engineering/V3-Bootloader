#include "handlers.h"
#include "boot_state.h"
#include "services/iso14229-1/11_ecuReset.h"

UDS_NRC_t x11_EcuReset(void *context, const void *request, void *response, void *user) {
    (void)context;
    (void)response;
    (void)user;

    const UDS_11_ER_Request *reset = request;

    if (!boot_state.authenticated) {
        return UDS_NRC_SECURITY_ACCESS_DENIED;
    }
    if (reset->resetType.raw != 1) {
        return UDS_NRC_SUBFUNCTION_NOT_SUPPORTED;
    }
    if (boot_state.transfer_active) {
        return UDS_NRC_REQUEST_SEQUENCE_ERROR;
    }

    boot_state.reset_pending = 1;
    return UDS_NRC_PR;
}
