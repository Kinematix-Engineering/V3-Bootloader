#pragma once

// Set to 1 to enforce SAMD51 debugger readout protection at startup.
// Clearing this option does not unlock a secured chip: debugger chip erase is
// required and erases the installed bootloader and application.
#ifndef BOOT_ENABLE_READOUT_PROTECTION
#define BOOT_ENABLE_READOUT_PROTECTION 1
#endif

#if BOOT_ENABLE_READOUT_PROTECTION != 0 && BOOT_ENABLE_READOUT_PROTECTION != 1
#error "BOOT_ENABLE_READOUT_PROTECTION must be 0 or 1"
#endif
