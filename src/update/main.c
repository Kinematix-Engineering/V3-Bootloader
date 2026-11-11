#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "sam.h"
#include "../hardware_addresses.h"
#include "v3_boot_table.h"
#include "table_preserve.h"

enum {
    ERASE_BLOCK = 8192,
    WRITE_UNIT = 16,
};

extern const uint8_t boot_image[];
extern const uint32_t boot_image_len;
extern uint8_t _etext, _srelocate, _erelocate;

static V3FactoryTable preserved;
static uint8_t block_data[ERASE_BLOCK] __attribute__((aligned(4)));

static bool wait_ready(void) {
    uint32_t timeout = 1000000u;
    while (!NVMCTRL->STATUS.bit.READY && --timeout) {
    }

    return timeout != 0;
}

static bool write_boot_block(uint32_t address, const uint8_t *data) {
    if (!wait_ready()) {
        return false;
    }

    NVMCTRL->ADDR.reg = address;
    NVMCTRL->CTRLB.reg = NVMCTRL_CTRLB_CMDEX_KEY | NVMCTRL_CTRLB_CMD_EB;

    if (!wait_ready()) {
        return false;
    }

    for (uint32_t offset = 0; offset < ERASE_BLOCK; offset += WRITE_UNIT) {
        NVMCTRL->CTRLB.reg = NVMCTRL_CTRLB_CMDEX_KEY | NVMCTRL_CTRLB_CMD_PBC;

        if (!wait_ready()) {
            return false;
        }

        volatile uint32_t *destination = (volatile uint32_t *)(address + offset);
        uint32_t words[4];
        memcpy(words, data + offset, sizeof(words));

        for (unsigned i = 0; i < 4; ++i) {
            destination[i] = words[i];
        }

        NVMCTRL->ADDR.reg = address + offset;
        NVMCTRL->CTRLB.reg = NVMCTRL_CTRLB_CMDEX_KEY | NVMCTRL_CTRLB_CMD_WQW;

        if (!wait_ready()) {
            return false;
        }
    }

    return memcmp((const void *)address, data, ERASE_BLOCK) == 0;
}

__attribute__((section(".ramfunc"), noinline, noreturn))
static void erase_self_from_ram(uint32_t image_end) {
    __disable_irq();

    for (uint32_t address = APP_START; address < image_end; address += ERASE_BLOCK) {
        while (!NVMCTRL->STATUS.bit.READY) {
        }

        NVMCTRL->ADDR.reg = address;
        NVMCTRL->CTRLB.reg = NVMCTRL_CTRLB_CMDEX_KEY | NVMCTRL_CTRLB_CMD_EB;

        while (!NVMCTRL->STATUS.bit.READY) {
        }
    }

    __DSB();
    NVIC_SystemReset();

    while (1) {
    }
}

int main(void) {
    __disable_irq();
    WDT->CTRLA.reg = 0;
    while (WDT->SYNCBUSY.reg) {
    }

    if (boot_image_len != BOOT_SIZE) {
        while (1) {
        }
    }

    boot_update_preserve_factory(&preserved, v3_factory_table_get());

    NVMCTRL->CTRLA.bit.CACHEDIS0 = 1;
    NVMCTRL->CTRLA.bit.CACHEDIS1 = 1;
    NVMCTRL->CTRLA.bit.WMODE = NVMCTRL_CTRLA_WMODE_MAN;
    NVMCTRL->CTRLB.reg = NVMCTRL_CTRLB_CMDEX_KEY | NVMCTRL_CTRLB_CMD_SBPDIS;

    if (!wait_ready()) {
        while (1) {
        }
    }

    for (uint32_t address = BOOT_START; address < BOOT_SIZE; address += ERASE_BLOCK) {
        memcpy(block_data, boot_image + address, ERASE_BLOCK);

        if (address + ERASE_BLOCK == BOOT_SIZE) {
            memcpy(block_data + V3_FACTORY_TABLE_ADDRESS - address, &preserved,
                   sizeof(preserved));
        }

        if (!write_boot_block(address, block_data)) {
            while (1) {
            }
        }
    }

    uint32_t loaded_end = (uint32_t)&_etext +
                          ((uint32_t)&_erelocate - (uint32_t)&_srelocate);
    uint32_t erase_end = (loaded_end + ERASE_BLOCK - 1u) & ~(ERASE_BLOCK - 1u);
    erase_self_from_ram(erase_end);
}
