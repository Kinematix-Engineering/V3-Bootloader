#include "nvm.h"
#include "../hardware_addresses.h"

bool flash_app_range(uint32_t address, uint32_t length) {
    return length != 0 && address >= APP_START && address < BANK_A_END &&
           length <= BANK_A_END - address;
}
