#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "usbd_core.h"

/* USB CDC-ACM + DFU runtime composite for a simple USB-to-UART bridge. */

#define USB_CDC_UART_EP_DATA_IN   0x81u
#define USB_CDC_UART_EP_DATA_OUT  0x01u
#define USB_CDC_UART_EP_CMD_IN    0x82u

#define USB_CDC_UART_DATA_MAX_PACKET_SIZE 64u
#define USB_CDC_UART_CMD_PACKET_SIZE      8u

#define USB_CDC_UART_CONFIG_DESC_SIZ (9u +  /* config */ \
	8u +                          /* IAD */ \
	9u + 5u + 5u + 4u + 5u + 7u + /* CDC comm + func + EP */ \
	9u + 7u + 7u +               /* CDC data + 2 EP */ \
	9u + 9u)                      /* DFU interface + functional */

#define USB_CDC_UART_INTERFACE_COMM 0u
#define USB_CDC_UART_INTERFACE_DATA 1u
#define USB_CDC_UART_INTERFACE_DFU  2u

typedef struct {
	USBD_HandleTypeDef *usb;
	bool dfu_detach_requested;
	uint8_t ep0_buf[16];
	uint8_t rx_buf[USB_CDC_UART_DATA_MAX_PACKET_SIZE];
	uint8_t tx_buf[USB_CDC_UART_DATA_MAX_PACKET_SIZE];
	bool tx_busy;

	/* line coding as in CDC spec */
	uint32_t dwDTERate;
	uint8_t bCharFormat;
	uint8_t bParityType;
	uint8_t bDataBits;

	USBD_SetupReqTypedef last_setup_request;
} USBD_CDC_UART_HandleTypeDef;

extern USBD_ClassTypeDef USBD_CDC_UART;

void USBD_CDC_UART_Init(USBD_CDC_UART_HandleTypeDef *hcdc, USBD_HandleTypeDef *pdev);
void USBD_CDC_UART_Task(USBD_CDC_UART_HandleTypeDef *hcdc);

bool USBD_CDC_UART_DfuDetachRequested(USBD_HandleTypeDef *pdev);

/* hooks for usbd_conf.c, return true if handled */
bool USBD_CDC_UART_CustomDeviceRequest(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
bool USBD_CDC_UART_CustomInterfaceRequest(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);

void USBD_CDC_UART_SuspendCallback(USBD_HandleTypeDef *pdev);
void USBD_CDC_UART_ResumeCallback(USBD_HandleTypeDef *pdev);
