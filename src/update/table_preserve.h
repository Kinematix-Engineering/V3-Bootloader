#pragma once

#include <stdint.h>
#include "v3_boot_table.h"

/** Copies the installed factory table before its flash block is erased. */
void boot_update_preserve_factory(V3FactoryTable *preserved,
                                  const V3FactoryTable *installed);
