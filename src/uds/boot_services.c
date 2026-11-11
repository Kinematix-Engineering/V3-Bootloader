#include "boot_services.h"
#include "boot_state.h"
#include "handlers.h"

#include <string.h>
#include "udslib.h"
#include "services/uds_std_services.h"

BootState boot_state;

static UdsSession session;

static const UdsServiceHandlerEntry services[] = {
    {x10_DiagnosticSessionControl, &UDS_10_DSC},
    {x11_EcuReset, &UDS_11_ER},
    {x22_ReadDataByIdentifier, &UDS_22_RDBI},
    {x27_SecurityAccess, &UDS_27_SA},
    {x31_RoutineControl, &UDS_31_RC},
    {x34_RequestDownload, &UDS_34_RD},
    {x36_TransferData, &UDS_36_TD},
    {x37_RequestTransferExit, &UDS_37_RTE},
};

uint32_t boot_be32(const uint8_t data[4]) {
    return ((uint32_t)data[0] << 24) |
           ((uint32_t)data[1] << 16) |
           ((uint32_t)data[2] << 8) |
           (uint32_t)data[3];
}

void boot_uds_init(void) {
    memset(&boot_state, 0, sizeof(boot_state));
    uds_server_init(&session, services, sizeof(services) / sizeof(services[0]));
}

void boot_uds_disconnect(void) {
    boot_uds_init();
}

size_t boot_uds_handle(const uint8_t *request, size_t request_len,
                       uint8_t *response, size_t response_capacity) {
    if (!request || !request_len || !response || response_capacity < 3) {
        return 0;
    }

    // The library accepts extra bytes for some fixed-length services.
    if ((request[0] == 0x22 && request_len != 3) ||
        ((request[0] == 0x10 || request[0] == 0x11) && request_len != 2)) {
        response[0] = 0x7f;
        response[1] = request[0];
        response[2] = UDS_NRC_IMLOIF;
        return 3;
    }

    UdsBufferCollection buffers = {
        .request = {(uint8_t *)request, request_len},
        .response = {response, response_capacity},
    };

    /* The library encoder treats suppressed x10 positives as encoding errors.
       Encode normally, then suppress only a successful programming response. */
    if (request[0] == 0x10 && (request[1] & 0x80u)) {
        uint8_t control[] = {0x10, (uint8_t)(request[1] & 0x7fu)};
        UdsBufferCollection control_buffers = {
            .request = {control, sizeof(control)},
            .response = {response, response_capacity},
        };
        size_t length = uds_server_process_request(&session, control_buffers, NULL);
        return length && response[0] == 0x50 ? 0 : length;
    }

    return uds_server_process_request(&session, buffers, NULL);
}

int boot_uds_take_reset(void) {
    int pending = boot_state.reset_pending;
    boot_state.reset_pending = 0;
    return pending;
}
