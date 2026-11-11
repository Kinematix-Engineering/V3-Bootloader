#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "v3_boot_table.h"
#include "table_preserve.h"

extern const V3BootTable v3_boot_table_image;
extern const V3FactoryTable v3_factory_table_image;

_Static_assert(sizeof(V3BootTable) == V3_BOOT_TABLE_SIZE,
               "Boot table must fill its reserved flash page");
_Static_assert(offsetof(V3FactoryTable, security) == V3_TABLE_PAGE_SIZE,
               "Security info must start on a page boundary");
_Static_assert(sizeof(V3FactoryTable) == V3_FACTORY_TABLE_SIZE,
               "Factory table must fill its reserved flash range");

static void expect_ff(const void *data, size_t length) {
    const uint8_t *bytes = data;

    for (size_t i = 0; i < length; ++i) {
        assert(bytes[i] == 0xff);
    }
}

int main(void) {
    assert((uintptr_t)v3_boot_table_get() == V3_BOOT_TABLE_ADDRESS);
    assert((uintptr_t)v3_factory_table_get() == V3_FACTORY_TABLE_ADDRESS);
    assert(V3_BOOT_TABLE_ADDRESS + V3_BOOT_TABLE_SIZE == V3_FACTORY_TABLE_ADDRESS);
    assert(V3_FACTORY_TABLE_ADDRESS + V3_FACTORY_TABLE_SIZE == 0x4000);

    const V3BootTable *boot_defaults = &v3_boot_table_image;
    const V3FactoryTable *factory_defaults = &v3_factory_table_image;
    assert(memcmp(boot_defaults->magic, "KMX", 3) == 0);
    assert(boot_defaults->format_version == V3_BOOT_TABLE_VERSION);
    assert(strcmp(boot_defaults->boot_version, "v3bl-1.2.0") == 0);
    expect_ff(boot_defaults->reserved, sizeof(boot_defaults->reserved));
    expect_ff(boot_defaults->padding, sizeof(boot_defaults->padding));
    expect_ff(factory_defaults, sizeof(*factory_defaults));

    V3FactoryTable installed = *factory_defaults;
    V3FactoryTable preserved;
    installed.part.hardware_part[0] = 'P';
    installed.security.secret_keys[0][0] = 0x12;
    boot_update_preserve_factory(&preserved, &installed);
    assert(memcmp(&preserved, &installed, sizeof(preserved)) == 0);

    uint8_t last_boot_block[8192];
    memset(last_boot_block, 0xff, sizeof(last_boot_block));
    V3BootTable updated_boot = *boot_defaults;
    strcpy(updated_boot.boot_version, "v3bl-1.3.0");
    memcpy(last_boot_block + V3_BOOT_TABLE_ADDRESS - 0x2000,
           &updated_boot, sizeof(updated_boot));
    memcpy(last_boot_block + V3_FACTORY_TABLE_ADDRESS - 0x2000,
           factory_defaults, sizeof(*factory_defaults));
    memcpy(last_boot_block + V3_FACTORY_TABLE_ADDRESS - 0x2000,
           &preserved, sizeof(preserved));

    assert(memcmp(last_boot_block + V3_BOOT_TABLE_ADDRESS - 0x2000,
                  &updated_boot, sizeof(updated_boot)) == 0);
    assert(memcmp(last_boot_block + V3_FACTORY_TABLE_ADDRESS - 0x2000,
                  &installed, sizeof(installed)) == 0);

    puts("boot and factory table layout and preservation passed");
}
