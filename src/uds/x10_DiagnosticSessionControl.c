#include "handlers.h"
#include "services/iso14229-1/10_diagnosticSessionControl.h"

UDS_NRC_t x10_DiagnosticSessionControl(void *context, const void *request,
                                      void *response, void *user) {
    (void)context;
    (void)user;
    const UDS_10_DSC_Request *control = request;
    UDS_10_DSC_Response *result = response;

    if (control->desiredSessionLevel.protocol.subfunction != UDS_DSC_SESSION_PROGRAMMING) {
        return UDS_NRC_SUBFUNCTION_NOT_SUPPORTED_IN_ACTIVE_SESSION;
    }

    /* The bootloader is already in programming session. */
    result->desiredSessionLevel.raw = UDS_DSC_SESSION_PROGRAMMING;
    return UDS_NRC_PR;
}
