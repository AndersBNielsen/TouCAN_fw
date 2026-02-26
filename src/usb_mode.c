#include "usb_mode.h"

#include "config.h"
#include "hal_include.h"

static usb_app_mode_t usb_mode = USB_APP_MODE_CAN;

void usb_app_mode_init_from_pins(void)
{
	usb_mode = USB_APP_MODE_CAN;

#if defined(USB_UART_MODESEL_GPIO_Port) && defined(USB_UART_MODESEL_Pin)
	/* Active-high: high at startup selects USB-to-UART mode. */
	GPIO_PinState state = HAL_GPIO_ReadPin(USB_UART_MODESEL_GPIO_Port, USB_UART_MODESEL_Pin);
	if (state == GPIO_PIN_SET) {
		usb_mode = USB_APP_MODE_UART;
	}
#endif
}

usb_app_mode_t usb_app_mode_get(void)
{
	return usb_mode;
}
