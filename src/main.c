#include "platform.h"
#include "usb/device.h"
#include "usb/transport.h"
#include "uds/boot_services.h"

int main(void) {
    platform_init();
    bool stay_in_boot = platform_should_enter_boot();
    uint32_t choke_ms = platform_boot_choke_ms();
    if (!stay_in_boot && !choke_ms) {
        platform_jump_to_app();
    }

    boot_uds_init();
    usb_transport_init();
    usb_device_init();
    const uint32_t choke_start = platform_millis();

    for (;;) {
        usb_transport_poll();
        stay_in_boot = stay_in_boot || boot_uds_programming_requested();
        if (usb_transport_take_reset()) {
            platform_reset_to_app();
        }
        if (!stay_in_boot &&
            (uint32_t)(platform_millis() - choke_start) >= choke_ms) {
            usb_device_shutdown();
            platform_jump_to_app();
        }
    }
}
