#include "handlers.h"
#include "v3_boot_table.h"
#include "KMX_UDS.h"
#include "../platform.h"
#include "protocol/uds_data_identifiers.h"
#include "services/iso14229-1/22_readDataByIdentifer.h"

#include <string.h>

enum { DIAGNOSTIC_SESSION = 0x02 };

static UDS_NRC_t copy_value(UDS_22_RDBI_Response *result,
                            const void *value, size_t length) {
    if (result->value_len < length) {
        return UDS_NRC_RESPONSE_TOO_LONG;
    }
    memcpy(result->value_buf, value, length);
    result->value_len = length;
    return UDS_NRC_PR;
}

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
    case UDS_DID_SYSTEM_SUPPLIER_ECU_HARDWARE_NUMBER: {
        const V3FactoryTable *factory = v3_factory_table_get();
        size_t length = 0;
        if (!v3_hardware_part_length(factory, &length)) {
            return UDS_NRC_CONDITIONS_NOT_CORRECT;
        }
        return copy_value(result, factory->part.hardware_part, length);
    }

    case UDS_DID_SYSTEM_SUPPLIER_ECU_HARDWARE_VERSION_NUMBER: {
        const V3PartInfo *part = &v3_factory_table_get()->part;
        if (!v3_hardware_version_is_valid(part)) {
            return UDS_NRC_CONDITIONS_NOT_CORRECT;
        }
        return copy_value(result, part->hardware_version, sizeof(part->hardware_version));
    }

    case UDS_DID_ECU_MANUFACTURING_DATE: {
        const V3PartInfo *part = &v3_factory_table_get()->part;
        if (!v3_manufacturing_date_is_valid(part)) {
            return UDS_NRC_CONDITIONS_NOT_CORRECT;
        }
        return copy_value(result, part->manufacturing_date, sizeof(part->manufacturing_date));
    }

    case UDS_DID_UDS_VERSION: {
        static const uint8_t version[] = {2, 0, 0, 0};
        return copy_value(result, version, sizeof(version));
    }

    case KMX_PID_MODULE_TYPE:
    case KMX_PID_MODULE_ADDRESS: {
        const V3BootTable *table = v3_boot_table_get();
        if (!v3_boot_table_is_valid(table)) {
            return UDS_NRC_CONDITIONS_NOT_CORRECT;
        }
        const uint8_t *value = read->data_identifier == KMX_PID_MODULE_TYPE ?
                               &table->module_type : &table->module_address;
        if (*value == 0xff) {
            return UDS_NRC_CONDITIONS_NOT_CORRECT;
        }
        return copy_value(result, value, 1);
    }

    case KMX_PID_DIAGNOSTIC_METHOD: {
        static const char method[] = "v3-boot-v1";
        return copy_value(result, method, sizeof(method) - 1);
    }

    case KMX_PID_UPTIME_MS: {
        const uint64_t uptime = platform_uptime_ms();
        uint8_t bytes[8];
        for (size_t i = 0; i < sizeof(bytes); ++i) {
            bytes[i] = (uint8_t)(uptime >> (8 * (sizeof(bytes) - 1 - i)));
        }
        return copy_value(result, bytes, sizeof(bytes));
    }

    case UDS_DID_ACTIVE_DIAGNOSTIC_SESSION:
        if (result->value_len < 1) {
            return UDS_NRC_RESPONSE_TOO_LONG;
        }

        result->value_buf[0] = DIAGNOSTIC_SESSION;
        result->value_len = 1;
        return UDS_NRC_PR;

    case UDS_DID_ECU_SERIAL_NUMBER: {
        static const char hex[] = "0123456789ABCDEF";
        uint8_t serial[16];
        if (result->value_len < 32) {
            return UDS_NRC_RESPONSE_TOO_LONG;
        }

        platform_serial(serial);
        for (size_t i = 0; i < sizeof(serial); ++i) {
            result->value_buf[2 * i] = (uint8_t)hex[serial[i] >> 4];
            result->value_buf[2 * i + 1] = (uint8_t)hex[serial[i] & 0x0f];
        }
        result->value_len = 32;
        return UDS_NRC_PR;
    }

    case UDS_DID_BOOT_SOFTWARE_IDENTIFICATION: {
        const V3BootTable *table = v3_boot_table_get();
        size_t version_length = 0;

        if (!v3_boot_version_length(table, &version_length)) {
            return UDS_NRC_CONDITIONS_NOT_CORRECT;
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
