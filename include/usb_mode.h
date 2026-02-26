#pragma once

#include <stdbool.h>

typedef enum {
	USB_APP_MODE_CAN = 0,
	USB_APP_MODE_UART = 1,
} usb_app_mode_t;

void usb_app_mode_init_from_pins(void);
usb_app_mode_t usb_app_mode_get(void);

static inline bool usb_app_mode_is_uart(void)
{
	return usb_app_mode_get() == USB_APP_MODE_UART;
}
