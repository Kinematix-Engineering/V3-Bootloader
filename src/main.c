#include "platform.h"
#include "usb/device.h"
#include "usb/transport.h"
#include "uds/boot_services.h"

int main(void) {
    platform_init();
    if (!platform_should_enter_boot()) {
        platform_jump_to_app();
    }

    boot_uds_init();
    usb_transport_init(platform_boot_session_response_pending());
    usb_device_init();

    for (;;) {
        usb_transport_poll();
        if (boot_uds_take_reset()) {
            platform_reset_to_app();
        }
    }
}
