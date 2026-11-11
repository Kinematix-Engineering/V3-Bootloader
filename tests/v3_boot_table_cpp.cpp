#include <cstddef>
#include <cstdint>

#include "v3_boot_table.h"

static_assert(sizeof(V3BootTable) == V3_BOOT_TABLE_SIZE);
static_assert(sizeof(V3FactoryTable) == V3_FACTORY_TABLE_SIZE);
static_assert(offsetof(V3FactoryTable, part) == 0);
static_assert(offsetof(V3FactoryTable, security) == V3_TABLE_PAGE_SIZE);

int main() {
    return reinterpret_cast<std::uintptr_t>(v3_boot_table_get()) != V3_BOOT_TABLE_ADDRESS ||
           reinterpret_cast<std::uintptr_t>(v3_factory_table_get()) != V3_FACTORY_TABLE_ADDRESS;
}
