#include "device.h"
#include "../platform.h"
#include <v3_boot_table.h>

#include <string.h>

enum {
    EP_CONTROL = 0,
    EP_DATA_IN = 1,
    EP_DATA_OUT = 2,
    EP_NOTIFY = 3,
    EP_COUNT = 4,
    PACKET_SIZE = 64,
};

typedef struct {
    uint8_t size;
    uint8_t offset;
    bool receiving;
    uint8_t buffer[PACKET_SIZE];
} EndpointBuffer;

static __attribute__((aligned(4))) UsbDeviceDescriptor endpoints[EP_COUNT];
static __attribute__((aligned(4))) EndpointBuffer ep_buffer[EP_COUNT];
static __attribute__((aligned(4))) uint8_t setup_buffer[PACKET_SIZE];
static __attribute__((aligned(4))) uint8_t zero_packet;
static uint8_t configuration;
static bool data_terminal_open;
static uint16_t control_length;

static const uint8_t device_descriptor[] = {
    18, 1, 0x00, 0x02, 0xef, 0x02, 0x01, 64,
    0x9a, 0x23, 0x31, 0x00, 0x02, 0x01, 1, 2, 3, 1,
};

static uint8_t configuration_descriptor[] = {
    9, 2, 75, 0, 2, 1, 0, 0x80, 50,
    8, 11, 0, 2, 2, 2, 1, 0,
    9, 4, 0, 0, 1, 2, 2, 1, 0,
    5, 0x24, 0, 0x10, 1,
    4, 0x24, 2, 6,
    5, 0x24, 6, 0, 1,
    5, 0x24, 1, 3, 1,
    7, 5, 0x80 | EP_NOTIFY, 3, 8, 0, 0xff,
    9, 4, 1, 0, 2, 0x0a, 0, 0, 0,
    7, 5, 0x80 | EP_DATA_IN, 2, 64, 0, 0,
    7, 5, EP_DATA_OUT, 2, 64, 0, 0,
};

static __attribute__((aligned(4))) uint8_t line_coding[7] = {
    (V3_BOOT_DIAG_BAUDRATE >> 0) & 0xffu,
    (V3_BOOT_DIAG_BAUDRATE >> 8) & 0xffu,
    (V3_BOOT_DIAG_BAUDRATE >> 16) & 0xffu,
    (V3_BOOT_DIAG_BAUDRATE >> 24) & 0xffu,
    0, 0, 8,
};
static uint8_t string_descriptor[70];

static void stall_control(void) {
    USB->DEVICE.DeviceEndpoint[EP_CONTROL].EPSTATUSSET.reg = USB_DEVICE_EPSTATUSSET_STALLRQ1;
}

static bool send_packet(const uint8_t *data, size_t length, unsigned ep) {
    if (length > 0xffffu) {
        return false;
    }

    if (!length) {
        data = &zero_packet;
    }

    if (length && length < PACKET_SIZE) {
        memcpy(ep_buffer[ep].buffer, data, length);
        data = ep_buffer[ep].buffer;
    }

    endpoints[ep].DeviceDescBank[1].ADDR.reg = (uint32_t)data;
    endpoints[ep].DeviceDescBank[1].PCKSIZE.bit.BYTE_COUNT = (uint16_t)length;
    endpoints[ep].DeviceDescBank[1].PCKSIZE.bit.MULTI_PACKET_SIZE = 0;
    endpoints[ep].DeviceDescBank[1].PCKSIZE.bit.AUTO_ZLP = length >= PACKET_SIZE;
    USB->DEVICE.DeviceEndpoint[ep].EPINTFLAG.reg = USB_DEVICE_EPINTFLAG_TRCPT1;
    USB->DEVICE.DeviceEndpoint[ep].EPSTATUSSET.reg = USB_DEVICE_EPSTATUSSET_BK1RDY;

    uint32_t start = platform_millis();
    while (!(USB->DEVICE.DeviceEndpoint[ep].EPINTFLAG.reg & USB_DEVICE_EPINTFLAG_TRCPT1)) {
        if ((uint32_t)(platform_millis() - start) > 1000u) {
            return false;
        }
    }

    return true;
}

static void send_control(const uint8_t *data, size_t length) {
    if (length > control_length) {
        length = control_length;
    }

    (void)send_packet(data, length, EP_CONTROL);
}

static void send_string(unsigned index) {
    static const char *const names[] = {NULL, "KEL", "V3 Bootloader", NULL};
    if (index == 0) {
        const uint8_t language[] = {4, 3, 0x09, 0x04};
        send_control(language, sizeof(language));
        return;
    }
    if (index > 3) {
        stall_control();
        return;
    }

    char serial_text[33];
    const char *name = names[index];

    if (index == 3) {
        static const char hex[] = "0123456789ABCDEF";
        uint8_t serial[16];
        platform_serial(serial);

        for (unsigned i = 0; i < 16; ++i) {
            serial_text[2 * i] = hex[serial[i] >> 4];
            serial_text[2 * i + 1] = hex[serial[i] & 15u];
        }

        serial_text[32] = 0;
        name = serial_text;
    }

    size_t len = strlen(name);
    string_descriptor[0] = (uint8_t)(2 + 2 * len);
    string_descriptor[1] = 3;

    for (size_t i = 0; i < len; ++i) {
        string_descriptor[2 + 2 * i] = (uint8_t)name[i];
        string_descriptor[3 + 2 * i] = 0;
    }

    send_control(string_descriptor, 2 + 2 * len);
}

static void configure_data_endpoints(void) {
    USB->DEVICE.DeviceEndpoint[EP_DATA_OUT].EPCFG.reg = USB_DEVICE_EPCFG_EPTYPE0(3);
    endpoints[EP_DATA_OUT].DeviceDescBank[0].PCKSIZE.bit.SIZE = 3;
    endpoints[EP_DATA_OUT].DeviceDescBank[0].ADDR.reg =
        (uint32_t)ep_buffer[EP_DATA_OUT].buffer;
    USB->DEVICE.DeviceEndpoint[EP_DATA_OUT].EPSTATUSSET.reg = USB_DEVICE_EPSTATUSSET_BK0RDY;

    USB->DEVICE.DeviceEndpoint[EP_DATA_IN].EPCFG.reg = USB_DEVICE_EPCFG_EPTYPE1(3);
    endpoints[EP_DATA_IN].DeviceDescBank[1].PCKSIZE.bit.SIZE = 3;
    USB->DEVICE.DeviceEndpoint[EP_DATA_IN].EPSTATUSCLR.reg = USB_DEVICE_EPSTATUSCLR_BK1RDY;

    USB->DEVICE.DeviceEndpoint[EP_NOTIFY].EPCFG.reg = USB_DEVICE_EPCFG_EPTYPE1(4);
    endpoints[EP_NOTIFY].DeviceDescBank[1].PCKSIZE.bit.SIZE = 0;
    USB->DEVICE.DeviceEndpoint[EP_NOTIFY].EPSTATUSCLR.reg = USB_DEVICE_EPSTATUSCLR_BK1RDY;
}

static void handle_setup(void) {
    USB->DEVICE.DeviceEndpoint[0].EPINTFLAG.reg = USB_DEVICE_EPINTFLAG_RXSTP;

    uint8_t request_type = setup_buffer[0];
    uint8_t request = setup_buffer[1];
    uint16_t value = (uint16_t)setup_buffer[2] | ((uint16_t)setup_buffer[3] << 8);
    uint16_t index = (uint16_t)setup_buffer[4] | ((uint16_t)setup_buffer[5] << 8);
    control_length = (uint16_t)setup_buffer[6] | ((uint16_t)setup_buffer[7] << 8);

    USB->DEVICE.DeviceEndpoint[0].EPSTATUSCLR.reg = USB_DEVICE_EPSTATUSCLR_BK0RDY;

    if (request_type == 0x80 && request == 6) {
        switch (value >> 8) {
        case 1:
            send_control(device_descriptor, sizeof(device_descriptor));
            return;
        case 2:
            send_control(configuration_descriptor, sizeof(configuration_descriptor));
            return;
        case 3:
            send_string(value & 0xffu);
            return;
        default:
            break;
        }
    }

    if (request_type == 0 && request == 5) {
        send_control(NULL, 0);
        USB->DEVICE.DADD.reg = USB_DEVICE_DADD_ADDEN | value;
        return;
    }

    if (request_type == 0 && request == 9 && value <= 1) {
        configuration = (uint8_t)value;
        data_terminal_open = false;
        send_control(NULL, 0);
        if (configuration) {
            configure_data_endpoints();
        }
        return;
    }

    if (request_type == 0x80 && request == 8) {
        send_control(&configuration, 1);
        return;
    }

    if ((request_type == 0x80 || request_type == 0x81) && request == 0) {
        static const uint8_t status[2] = {0, 0};
        send_control(status, 2);
        return;
    }

    if (request_type == 0x81 && request == 10) {
        static const uint8_t alternate = 0;
        send_control(&alternate, 1);
        return;
    }

    if (request_type == 0x01 && request == 11 && value == 0) {
        send_control(NULL, 0);
        return;
    }

    if (request_type == 0xa1 && request == 0x21 && index == 0) {
        send_control(line_coding, sizeof(line_coding));
        return;
    }

    if (request_type == 0x21 && request == 0x20 && index == 0) {
        if (control_length != sizeof(line_coding)) {
            stall_control();
            return;
        }

        endpoints[0].DeviceDescBank[0].ADDR.reg = (uint32_t)line_coding;
        endpoints[0].DeviceDescBank[0].PCKSIZE.bit.BYTE_COUNT = 0;
        endpoints[0].DeviceDescBank[0].PCKSIZE.bit.MULTI_PACKET_SIZE = sizeof(line_coding);
        USB->DEVICE.DeviceEndpoint[0].EPINTFLAG.reg = USB_DEVICE_EPINTFLAG_TRCPT0;
        USB->DEVICE.DeviceEndpoint[0].EPSTATUSCLR.reg = USB_DEVICE_EPSTATUSCLR_BK0RDY;

        uint32_t start = platform_millis();
        while (!(USB->DEVICE.DeviceEndpoint[0].EPINTFLAG.reg & USB_DEVICE_EPINTFLAG_TRCPT0)) {
            if ((uint32_t)(platform_millis() - start) > 1000u) {
                stall_control();
                return;
            }
        }

        USB->DEVICE.DeviceEndpoint[0].EPINTFLAG.reg = USB_DEVICE_EPINTFLAG_TRCPT0;
        endpoints[0].DeviceDescBank[0].ADDR.reg = (uint32_t)setup_buffer;
        endpoints[0].DeviceDescBank[0].PCKSIZE.bit.MULTI_PACKET_SIZE = 8;
        endpoints[0].DeviceDescBank[0].PCKSIZE.bit.BYTE_COUNT = 0;

        // The CDC baud setting is accepted; transport framing stays byte-oriented.
        send_control(NULL, 0);
        return;
    }

    if (request_type == 0x21 && request == 0x22 && index == 0) {
        data_terminal_open = (value & 1u) != 0;
        send_control(NULL, 0);
        return;
    }

    stall_control();
}

void usb_device_init(void) {
    PORT->Group[0].PINCFG[24].bit.PMUXEN = 1;
    PORT->Group[0].PINCFG[25].bit.PMUXEN = 1;
    PORT->Group[0].PMUX[12].reg = (uint8_t)((MUX_PA24H_USB_DM & 15u) |
                                            ((MUX_PA25H_USB_DP & 15u) << 4));
    GCLK->PCHCTRL[USB_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK0_Val |
                                    GCLK_PCHCTRL_CHEN;
    MCLK->AHBMASK.bit.USB_ = 1;
    MCLK->APBBMASK.bit.USB_ = 1;

    USB->HOST.CTRLA.bit.SWRST = 1;
    while (USB->HOST.SYNCBUSY.bit.SWRST) {
    }

    uint32_t transn = (*(const uint32_t *)USB_FUSES_TRANSN_ADDR &
                       USB_FUSES_TRANSN_Msk) >> USB_FUSES_TRANSN_Pos;
    uint32_t transp = (*(const uint32_t *)USB_FUSES_TRANSP_ADDR &
                       USB_FUSES_TRANSP_Msk) >> USB_FUSES_TRANSP_Pos;
    uint32_t trim = (*(const uint32_t *)USB_FUSES_TRIM_ADDR &
                     USB_FUSES_TRIM_Msk) >> USB_FUSES_TRIM_Pos;

    USB->HOST.PADCAL.bit.TRANSN = transn == 31u ? 5u : transn;
    USB->HOST.PADCAL.bit.TRANSP = transp == 31u ? 29u : transp;
    USB->HOST.PADCAL.bit.TRIM = trim == 7u ? 3u : trim;

    USB->HOST.CTRLA.bit.MODE = 0;
    USB->HOST.CTRLA.bit.RUNSTDBY = 1;
    USB->HOST.DESCADD.reg = (uint32_t)endpoints;
    USB->DEVICE.CTRLB.bit.SPDCONF = USB_DEVICE_CTRLB_SPDCONF_FS_Val;
    USB->DEVICE.CTRLB.reg &= ~USB_DEVICE_CTRLB_DETACH;

    memset(endpoints, 0, sizeof(endpoints));
    memset(ep_buffer, 0, sizeof(ep_buffer));
    configuration = 0;
    data_terminal_open = false;

    USB->HOST.CTRLA.bit.ENABLE = 1;
}

bool usb_device_poll(void) {
    if (USB->DEVICE.INTFLAG.reg & USB_DEVICE_INTFLAG_EORST) {
        USB->DEVICE.INTFLAG.reg = USB_DEVICE_INTFLAG_EORST;
        configuration = 0;
        data_terminal_open = false;
        memset(ep_buffer, 0, sizeof(ep_buffer));

        USB->DEVICE.DADD.reg = USB_DEVICE_DADD_ADDEN;
        USB->DEVICE.DeviceEndpoint[0].EPCFG.reg =
            USB_DEVICE_EPCFG_EPTYPE0(1) | USB_DEVICE_EPCFG_EPTYPE1(1);
        USB->DEVICE.DeviceEndpoint[0].EPSTATUSSET.reg = USB_DEVICE_EPSTATUSSET_BK0RDY;
        USB->DEVICE.DeviceEndpoint[0].EPSTATUSCLR.reg = USB_DEVICE_EPSTATUSCLR_BK1RDY;
        endpoints[0].DeviceDescBank[0].PCKSIZE.bit.SIZE = 3;
        endpoints[0].DeviceDescBank[1].PCKSIZE.bit.SIZE = 3;
        endpoints[0].DeviceDescBank[0].ADDR.reg = (uint32_t)setup_buffer;
        endpoints[0].DeviceDescBank[1].ADDR.reg = (uint32_t)ep_buffer[0].buffer;
        endpoints[0].DeviceDescBank[0].PCKSIZE.bit.MULTI_PACKET_SIZE = 8;
        endpoints[0].DeviceDescBank[0].PCKSIZE.bit.BYTE_COUNT = 0;
        USB->DEVICE.DeviceEndpoint[0].EPSTATUSCLR.reg = USB_DEVICE_EPSTATUSCLR_BK0RDY;
    }

    if (USB->DEVICE.DeviceEndpoint[0].EPINTFLAG.reg & USB_DEVICE_EPINTFLAG_RXSTP) {
        handle_setup();
    }

    return configuration != 0;
}

bool usb_device_ready(void) {
    return configuration != 0 && data_terminal_open;
}

size_t usb_device_read(uint8_t *data, size_t capacity) {
    if (!configuration || !data || !capacity) {
        return 0;
    }

    EndpointBuffer *buffer = &ep_buffer[EP_DATA_OUT];

    if (buffer->offset < buffer->size) {
        size_t available = buffer->size - buffer->offset;
        if (available > capacity) {
            available = capacity;
        }

        memcpy(data, buffer->buffer + buffer->offset, available);
        buffer->offset += (uint8_t)available;
        return available;
    }

    if (!buffer->receiving) {
        endpoints[EP_DATA_OUT].DeviceDescBank[0].ADDR.reg = (uint32_t)buffer->buffer;
        endpoints[EP_DATA_OUT].DeviceDescBank[0].PCKSIZE.bit.BYTE_COUNT = 0;
        endpoints[EP_DATA_OUT].DeviceDescBank[0].PCKSIZE.bit.MULTI_PACKET_SIZE = 0;
        USB->DEVICE.DeviceEndpoint[EP_DATA_OUT].EPSTATUSCLR.reg = USB_DEVICE_EPSTATUSCLR_BK0RDY;
        buffer->receiving = true;
    }

    if (!(USB->DEVICE.DeviceEndpoint[EP_DATA_OUT].EPINTFLAG.reg &
          USB_DEVICE_EPINTFLAG_TRCPT0)) {
        return 0;
    }

    buffer->size = endpoints[EP_DATA_OUT].DeviceDescBank[0].PCKSIZE.bit.BYTE_COUNT;
    buffer->offset = 0;
    buffer->receiving = false;
    USB->DEVICE.DeviceEndpoint[EP_DATA_OUT].EPINTFLAG.reg = USB_DEVICE_EPINTFLAG_TRCPT0;

    return usb_device_read(data, capacity);
}

bool usb_device_write(const uint8_t *data, size_t length) {
    return configuration && data && send_packet(data, length, EP_DATA_IN);
}
