#include "platform.h"
#include "v3_boot_table.h"

static volatile uint32_t milliseconds;
static volatile uint32_t *const boot_magic = (volatile uint32_t *)BOOT_MAGIC_ADDRESS;
static volatile uint8_t *const boot_request = (volatile uint8_t *)V3_BOOT_REQUEST_ADDRESS;
static const uint32_t double_tap_magic = 0xf01669efu;
static const uint32_t quick_boot_magic = 0xf02669efu;
static bool boot_session_response_pending;

void SysTick_Handler(void) {
    ++milliseconds;
}

void platform_init(void) {
    WDT->CTRLA.reg = 0;
    while (WDT->SYNCBUSY.reg) {
    }

    NVMCTRL->CTRLA.bit.AUTOWS = 1;

    GCLK->CTRLA.bit.SWRST = 1;
    while (GCLK->SYNCBUSY.bit.SWRST) {
    }

    GCLK->GENCTRL[0].reg = GCLK_GENCTRL_SRC(GCLK_GENCTRL_SRC_OSCULP32K) |
                            GCLK_GENCTRL_GENEN;
    while (GCLK->SYNCBUSY.bit.GENCTRL0) {
    }

    OSCCTRL->DFLLCTRLA.reg = 0;
    OSCCTRL->DFLLMUL.reg = OSCCTRL_DFLLMUL_CSTEP(1) |
                           OSCCTRL_DFLLMUL_FSTEP(1) |
                           OSCCTRL_DFLLMUL_MUL(48000);
    while (OSCCTRL->DFLLSYNC.bit.DFLLMUL) {
    }

    OSCCTRL->DFLLCTRLB.reg = 0;
    while (OSCCTRL->DFLLSYNC.bit.DFLLCTRLB) {
    }

    OSCCTRL->DFLLCTRLA.bit.ENABLE = 1;
    while (OSCCTRL->DFLLSYNC.bit.ENABLE) {
    }

    OSCCTRL->DFLLVAL.reg = OSCCTRL->DFLLVAL.reg;
    while (OSCCTRL->DFLLSYNC.bit.DFLLVAL) {
    }

    OSCCTRL->DFLLCTRLB.reg = OSCCTRL_DFLLCTRLB_WAITLOCK |
                             OSCCTRL_DFLLCTRLB_CCDIS |
                             OSCCTRL_DFLLCTRLB_USBCRM;
    while (!OSCCTRL->STATUS.bit.DFLLRDY) {
    }

    GCLK->GENCTRL[0].reg = GCLK_GENCTRL_SRC(GCLK_GENCTRL_SRC_DFLL) |
                           GCLK_GENCTRL_IDC | GCLK_GENCTRL_GENEN;
    while (GCLK->SYNCBUSY.bit.GENCTRL0) {
    }

    MCLK->CPUDIV.reg = MCLK_CPUDIV_DIV_DIV1;
    SysTick_Config(48000u);
}

uint32_t platform_millis(void) {
    return milliseconds;
}

void platform_delay(uint32_t ms) {
    const uint32_t start = milliseconds;
    while ((uint32_t)(milliseconds - start) < ms) {
    }
}

void platform_serial(uint8_t serial[16]) {
    static const uint32_t addresses[4] = {
        SAMD51_SERIAL_WORD0_ADDRESS,
        SAMD51_SERIAL_WORD1_ADDRESS,
        SAMD51_SERIAL_WORD2_ADDRESS,
        SAMD51_SERIAL_WORD3_ADDRESS,
    };

    for (unsigned i = 0; i < 4; ++i) {
        const uint32_t word = *(const volatile uint32_t *)addresses[i];

        for (unsigned j = 0; j < 4; ++j) {
            serial[4 * i + j] = (uint8_t)(word >> (8 * j));
        }
    }
}

bool platform_app_valid(void) {
    const uint32_t *vectors = (const uint32_t *)APP_START;
    const uint32_t sp = vectors[0];
    const uint32_t pc = vectors[1];

    return sp >= HSRAM_ADDR && sp <= HSRAM_ADDR + HSRAM_SIZE &&
           pc >= APP_START && pc < BANK_A_END && (pc & 1u) != 0;
}

bool platform_should_enter_boot(void) {
    boot_session_response_pending = RSTC->RCAUSE.reg == RSTC_RCAUSE_SYST &&
                                    *boot_request == V3_BOOT_REQUEST_MARKER;
    *boot_request = 0;

    if (boot_session_response_pending) {
        *boot_magic = 0;
        return true;
    }

    if (!platform_app_valid()) {
        return true;
    }

    if (*boot_magic == quick_boot_magic) {
        *boot_magic = 0;
        return false;
    }

    if (RSTC->RCAUSE.bit.POR) {
        *boot_magic = 0;
        return false;
    }

    if (*boot_magic == double_tap_magic) {
        *boot_magic = 0;
        return true;
    }

    *boot_magic = double_tap_magic;
    platform_delay(500);
    *boot_magic = 0;
    return false;
}

bool platform_boot_session_response_pending(void) {
    return boot_session_response_pending;
}

void platform_jump_to_app(void) {
    const uint32_t *vectors = (const uint32_t *)APP_START;

    __disable_irq();
    SysTick->CTRL = 0;
    SCB->VTOR = APP_START;
    __set_MSP(vectors[0]);
    ((void (*)(void))vectors[1])();

    while (1) {
    }
}

void platform_reset_to_app(void) {
    *boot_magic = quick_boot_magic;
    NVIC_SystemReset();

    while (1) {
    }
}

void platform_reset_to_boot(void) {
    *boot_magic = double_tap_magic;
    NVIC_SystemReset();

    while (1) {
    }
}
