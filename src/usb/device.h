#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** Initializes the SAMD51 USB peripheral as a CDC device. */
void usb_device_init(void);

/** Services USB control traffic and reports configuration state. */
bool usb_device_poll(void);

/** Reports whether a configured CDC connection has its data terminal open. */
bool usb_device_ready(void);

/** Reads currently available bytes from the CDC OUT endpoint. */
size_t usb_device_read(uint8_t *data, size_t capacity);

/** Writes bytes to the CDC IN endpoint. */
bool usb_device_write(const uint8_t *data, size_t length);
