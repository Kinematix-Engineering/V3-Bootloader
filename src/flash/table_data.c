#include "v3_boot_table.h"

__attribute__((section(".v3_boot_table"), used))
const V3BootTable v3_boot_table_image = {
    .magic = {'K', 'M', 'X'},
    .format_version = V3_BOOT_TABLE_VERSION,
    .boot_version = "v3bl-1.2.0",
    .reserved = {[0 ... 63] = 0xff},
    .padding = {[0 ... 411] = 0xff},
};

__attribute__((section(".v3_factory_table"), used))
const V3FactoryTable v3_factory_table_image = {
    .part = {
        .hardware_part = {[0 ... 31] = (char)0xff},
        .hardware_version = {0xff, 0xff, 0xff},
        .reserved = {0xff},
        .manufacturing_date = {[0 ... 7] = 0xff},
        .padding = {[0 ... 467] = 0xff},
    },
    .security = {
        .secret_keys = {[0 ... 15] = {[0 ... 15] = 0xff}},
        .public_keys = {[0 ... 1] = {[0 ... 31] = 0xff}},
        .padding = {[0 ... 191] = 0xff},
    },
};
