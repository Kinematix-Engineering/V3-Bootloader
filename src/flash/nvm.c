#include "nvm.h"
#include "checksum.h"
#include "../platform.h"

#include <string.h>

enum {
    ERASE_BLOCK_SIZE = 8192,
    WRITE_UNIT_SIZE = 16,
};

static bool wait_ready(void) {
    uint32_t timeout = 1000000u;
    while (!NVMCTRL->STATUS.bit.READY && --timeout) {
    }

    return timeout != 0;
}

bool flash_erase_app(uint32_t address, uint32_t length) {
    if (!flash_app_range(address, length) ||
        address % ERASE_BLOCK_SIZE || length % ERASE_BLOCK_SIZE) {
        return false;
    }

    for (uint32_t current = address; current < address + length; current += ERASE_BLOCK_SIZE) {
        if (!wait_ready()) {
            return false;
        }

        NVMCTRL->ADDR.reg = current;
        NVMCTRL->CTRLB.reg = NVMCTRL_CTRLB_CMDEX_KEY | NVMCTRL_CTRLB_CMD_EB;

        if (!wait_ready()) {
            return false;
        }
    }

    return true;
}

bool flash_write_app(uint32_t address, const uint8_t *data, size_t length) {
    if (!data || !flash_app_range(address, (uint32_t)length) ||
        address % WRITE_UNIT_SIZE || length % WRITE_UNIT_SIZE) {
        return false;
    }

    NVMCTRL->CTRLA.bit.CACHEDIS0 = 1;
    NVMCTRL->CTRLA.bit.CACHEDIS1 = 1;
    NVMCTRL->CTRLA.bit.WMODE = NVMCTRL_CTRLA_WMODE_MAN;

    for (size_t offset = 0; offset < length; offset += WRITE_UNIT_SIZE) {
        if (!wait_ready()) {
            return false;
        }

        NVMCTRL->CTRLB.reg = NVMCTRL_CTRLB_CMDEX_KEY | NVMCTRL_CTRLB_CMD_PBC;

        if (!wait_ready()) {
            return false;
        }

        uint32_t words[4];
        memcpy(words, data + offset, sizeof(words));

        volatile uint32_t *dest = (volatile uint32_t *)(address + offset);
        for (unsigned i = 0; i < 4; ++i) {
            dest[i] = words[i];
        }

        NVMCTRL->ADDR.reg = address + (uint32_t)offset;
        NVMCTRL->CTRLB.reg = NVMCTRL_CTRLB_CMDEX_KEY | NVMCTRL_CTRLB_CMD_WQW;

        if (!wait_ready() ||
            memcmp((const void *)(address + offset), data + offset, WRITE_UNIT_SIZE)) {
            return false;
        }
    }

    return true;
}

void flash_app_checksums(uint32_t length, uint32_t *xor32, uint32_t *sum32) {
    flash_checksum_words((const uint8_t *)APP_START, length, xor32, sum32);
}
