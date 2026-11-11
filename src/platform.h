#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "sam.h"
#include "hardware_addresses.h"

/** Sets up clocks, flash wait states, the watchdog, and the millisecond tick. */
void platform_init(void);

/** Returns milliseconds since bootloader initialization. */
uint32_t platform_millis(void);

/** Waits for the requested number of milliseconds. */
void platform_delay(uint32_t ms);

/** Reads the SAMD51 factory serial as sixteen protocol-order bytes. */
void platform_serial(uint8_t serial[16]);

/** Checks whether application reset vectors point to usable RAM and flash. */
bool platform_app_valid(void);

/** Transfers execution to the installed application. */
void platform_jump_to_app(void) __attribute__((noreturn));

/** Resets the device and starts a valid application. */
void platform_reset_to_app(void) __attribute__((noreturn));

/** Resets the device into bootloader mode. */
void platform_reset_to_boot(void) __attribute__((noreturn));

/** Applies the ASW request, double-tap, and application-validity boot decision. */
bool platform_should_enter_boot(void);

/** Reports whether an ASW boot request requires a deferred 50 02 response. */
bool platform_boot_session_response_pending(void);
