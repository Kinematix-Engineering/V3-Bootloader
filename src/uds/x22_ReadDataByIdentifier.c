#include "handlers.h"
#include "v3_boot_table.h"
#include "../platform.h"
#include "protocol/uds_data_identifiers.h"
#include "services/iso14229-1/22_readDataByIdentifer.h"

#include <string.h>

enum { DIAGNOSTIC_SESSION = 0x02 };

UDS_NRC_t x22_ReadDataByIdentifier(void *context, const void *request,
                                    void *response, void *user) {
    (void)context;
    (void)user;

    const UDS_22_RDBI_Request *read = request;
    UDS_22_RDBI_Response *result = response;

    if (!result->value_buf) {
        return UDS_NRC_RESPONSE_TOO_LONG;
    }

    switch (read->data_identifier) {
    case UDS_DID_ACTIVE_DIAGNOSTIC_SESSION:
        if (result->value_len < 1) {
            return UDS_NRC_RESPONSE_TOO_LONG;
        }

        result->value_buf[0] = DIAGNOSTIC_SESSION;
        result->value_len = 1;
        return UDS_NRC_PR;

    case UDS_DID_ECU_SERIAL_NUMBER:
        if (result->value_len < 16) {
            return UDS_NRC_RESPONSE_TOO_LONG;
        }

        platform_serial(result->value_buf);
        result->value_len = 16;
        return UDS_NRC_PR;

    case UDS_DID_BOOT_SOFTWARE_IDENTIFICATION: {
        const V3BootTable *table = v3_boot_table_get();
        size_t version_length = 0;

        while (version_length < sizeof(table->boot_version) &&
               table->boot_version[version_length] != '\0') {
            ++version_length;
        }

        if (result->value_len < version_length) {
            return UDS_NRC_RESPONSE_TOO_LONG;
        }

        memcpy(result->value_buf, table->boot_version, version_length);
        result->value_len = version_length;
        return UDS_NRC_PR;
    }

    default:
        return UDS_NRC_REQUEST_OUT_OF_RANGE;
    }
}
