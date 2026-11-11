#include "table_preserve.h"

#include <string.h>

void boot_update_preserve_factory(V3FactoryTable *preserved,
                                  const V3FactoryTable *installed) {
    memcpy(preserved, installed, sizeof(*preserved));
}
