#include "usbd_cdc_uart.h"

#include <string.h>

#include "config.h"
#include "uart_bridge.h"
#include "usbd_ctlreq.h"
#include "usbd_def.h"
#include "usbd_desc.h"
#include "usbd_gs_can.h"
#include "usbd_ioreq.h"

/* Minimal CDC-ACM (no call mgmt), with DFU runtime interface preserved. */

#define CDC_REQ_SEND_ENCAPSULATED_COMMAND 0x00u
#define CDC_REQ_GET_ENCAPSULATED_RESPONSE 0x01u
#define CDC_REQ_SET_LINE_CODING 0x20u
#define CDC_REQ_GET_LINE_CODING 0x21u
#define CDC_REQ_SET_CONTROL_LINE_STATE 0x22u
#define CDC_REQ_SEND_BREAK 0x23u

/* Configuration Descriptor */
static const uint8_t USBD_CDC_UART_CfgDesc[USB_CDC_UART_CONFIG_DESC_SIZ] = {
	/* Configuration Descriptor */
	0x09,
	USB_DESC_TYPE_CONFIGURATION,
	LOBYTE(USB_CDC_UART_CONFIG_DESC_SIZ),
	HIBYTE(USB_CDC_UART_CONFIG_DESC_SIZ),
	0x03, /* bNumInterfaces: CDC (2) + DFU (1) */
	0x01,
	USBD_IDX_CONFIG_STR,
	0x80,
	0x4B,

	/* IAD: CDC function (interfaces 0-1) */
	0x08,
	0x0B, /* USB_DESC_TYPE_IAD */
	USB_CDC_UART_INTERFACE_COMM,
	0x02, /* bInterfaceCount */
	0x02, /* CDC */
	0x02, /* ACM */
	0x01,
	0x00,

	/* Interface Descriptor: CDC Communication */
	0x09,
	USB_DESC_TYPE_INTERFACE,
	USB_CDC_UART_INTERFACE_COMM,
	0x00,
	0x01, /* one interrupt IN */
	0x02, /* CDC */
	0x02, /* ACM */
	0x01,
	0x00,

	/* Header Functional Descriptor */
	0x05,
	0x24,
	0x00,
	0x10, 0x01,

	/* Call Management Functional Descriptor */
	0x05,
	0x24,
	0x01,
	0x00,
	USB_CDC_UART_INTERFACE_DATA,

	/* ACM Functional Descriptor */
	0x04,
	0x24,
	0x02,
	0x02,

	/* Union Functional Descriptor */
	0x05,
	0x24,
	0x06,
	USB_CDC_UART_INTERFACE_COMM,
	USB_CDC_UART_INTERFACE_DATA,

	/* Endpoint Descriptor: CDC Command IN */
	0x07,
	USB_DESC_TYPE_ENDPOINT,
	USB_CDC_UART_EP_CMD_IN,
	0x03, /* interrupt */
	LOBYTE(USB_CDC_UART_CMD_PACKET_SIZE),
	HIBYTE(USB_CDC_UART_CMD_PACKET_SIZE),
	0x10,

	/* Interface Descriptor: CDC Data */
	0x09,
	USB_DESC_TYPE_INTERFACE,
	USB_CDC_UART_INTERFACE_DATA,
	0x00,
	0x02,
	0x0A, /* data */
	0x00,
	0x00,
	0x00,

	/* Endpoint Descriptor: CDC Data OUT */
	0x07,
	USB_DESC_TYPE_ENDPOINT,
	USB_CDC_UART_EP_DATA_OUT,
	0x02, /* bulk */
	LOBYTE(USB_CDC_UART_DATA_MAX_PACKET_SIZE),
	HIBYTE(USB_CDC_UART_DATA_MAX_PACKET_SIZE),
	0x00,

	/* Endpoint Descriptor: CDC Data IN */
	0x07,
	USB_DESC_TYPE_ENDPOINT,
	USB_CDC_UART_EP_DATA_IN,
	0x02, /* bulk */
	LOBYTE(USB_CDC_UART_DATA_MAX_PACKET_SIZE),
	HIBYTE(USB_CDC_UART_DATA_MAX_PACKET_SIZE),
	0x00,

	/* DFU Interface Descriptor */
	0x09,
	USB_DESC_TYPE_INTERFACE,
	USB_CDC_UART_INTERFACE_DFU,
	0x00,
	0x00,
	0xFE,
	0x01,
	0x01,
	DFU_INTERFACE_STR_INDEX,

	/* Run-Time DFU Functional Descriptor */
	0x09,
	0x21,
	0x0B,
	0xFF, 0x00,
	0x00, 0x08,
	0x1a, 0x01,
};

static uint8_t USBD_CDC_UART_InitIntf(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t USBD_CDC_UART_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t USBD_CDC_UART_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static uint8_t USBD_CDC_UART_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t USBD_CDC_UART_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t USBD_CDC_UART_EP0_RxReady(USBD_HandleTypeDef *pdev);
static uint8_t *USBD_CDC_UART_GetCfgDesc(uint16_t *length);
static uint8_t *USBD_CDC_UART_GetStrDesc(USBD_HandleTypeDef *pdev, uint8_t index, uint16_t *length);

USBD_ClassTypeDef USBD_CDC_UART = {
	.Init = USBD_CDC_UART_InitIntf,
	.DeInit = USBD_CDC_UART_DeInit,
	.Setup = USBD_CDC_UART_Setup,
	.EP0_RxReady = USBD_CDC_UART_EP0_RxReady,
	.DataIn = USBD_CDC_UART_DataIn,
	.DataOut = USBD_CDC_UART_DataOut,
	.GetHSConfigDescriptor = USBD_CDC_UART_GetCfgDesc,
	.GetFSConfigDescriptor = USBD_CDC_UART_GetCfgDesc,
	.GetOtherSpeedConfigDescriptor = USBD_CDC_UART_GetCfgDesc,
	.GetUsrStrDescriptor = USBD_CDC_UART_GetStrDesc,
};

static USBD_CDC_UART_HandleTypeDef *cdc_handle(USBD_HandleTypeDef *pdev)
{
	return (USBD_CDC_UART_HandleTypeDef *)pdev->pClassData;
}

void USBD_CDC_UART_Init(USBD_CDC_UART_HandleTypeDef *hcdc, USBD_HandleTypeDef *pdev)
{
	memset(hcdc, 0, sizeof(*hcdc));
	pdev->pClassData = hcdc;
	hcdc->usb = pdev;

	/* Default line coding: 115200 8N1 */
	hcdc->dwDTERate = 115200;
	hcdc->bCharFormat = 0; /* 1 stop */
	hcdc->bParityType = 0; /* none */
	hcdc->bDataBits = 8;

	uart_line_coding_t coding = {
		.baudrate = hcdc->dwDTERate,
		.data_bits = hcdc->bDataBits,
		.stop_bits = 1,
		.parity = 0,
	};
	uart_bridge_init(&coding);
}

static uint8_t USBD_CDC_UART_InitIntf(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
	UNUSED(cfgidx);

	USBD_LL_OpenEP(pdev, USB_CDC_UART_EP_DATA_IN, USBD_EP_TYPE_BULK, USB_CDC_UART_DATA_MAX_PACKET_SIZE);
	USBD_LL_OpenEP(pdev, USB_CDC_UART_EP_DATA_OUT, USBD_EP_TYPE_BULK, USB_CDC_UART_DATA_MAX_PACKET_SIZE);
	USBD_LL_OpenEP(pdev, USB_CDC_UART_EP_CMD_IN, USBD_EP_TYPE_INTR, USB_CDC_UART_CMD_PACKET_SIZE);

	USBD_LL_PrepareReceive(pdev, USB_CDC_UART_EP_DATA_OUT, cdc_handle(pdev)->rx_buf, USB_CDC_UART_DATA_MAX_PACKET_SIZE);
	return USBD_OK;
}

static uint8_t USBD_CDC_UART_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
	UNUSED(cfgidx);

	USBD_LL_CloseEP(pdev, USB_CDC_UART_EP_DATA_IN);
	USBD_LL_CloseEP(pdev, USB_CDC_UART_EP_DATA_OUT);
	USBD_LL_CloseEP(pdev, USB_CDC_UART_EP_CMD_IN);
	return USBD_OK;
}

static uint8_t USBD_CDC_UART_DFU_Request(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
	USBD_CDC_UART_HandleTypeDef *hcdc = cdc_handle(pdev);

	switch (req->bRequest) {
		case 0: /* DETACH */
			hcdc->dfu_detach_requested = true;
			break;
		case 3: /* GET_STATUS */
			hcdc->ep0_buf[0] = 0x00;
			hcdc->ep0_buf[1] = 0x00;
			hcdc->ep0_buf[2] = 0x00;
			hcdc->ep0_buf[3] = 0x00;
			hcdc->ep0_buf[4] = 0x00; /* appIDLE */
			hcdc->ep0_buf[5] = 0xFF;
			USBD_CtlSendData(pdev, hcdc->ep0_buf, 6);
			break;
		default:
			USBD_CtlError(pdev, req);
	}

	return USBD_OK;
}

static void USBD_CDC_UART_ApplyLineCoding(USBD_CDC_UART_HandleTypeDef *hcdc)
{
	uart_line_coding_t coding = {
		.baudrate = hcdc->dwDTERate,
		.data_bits = hcdc->bDataBits,
		.stop_bits = (hcdc->bCharFormat == 2) ? 2 : 1,
		.parity = (hcdc->bParityType == 1) ? 1 : (hcdc->bParityType == 2 ? 2 : 0),
	};
	uart_bridge_set_line_coding(&coding);
}

static uint8_t USBD_CDC_UART_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
	USBD_CDC_UART_HandleTypeDef *hcdc = cdc_handle(pdev);

	switch (req->bmRequest & USB_REQ_TYPE_MASK) {
		case USB_REQ_TYPE_CLASS: {
			uint8_t req_rcpt = req->bmRequest & 0x1F;
			if (req_rcpt != USB_REQ_RECIPIENT_INTERFACE) {
				USBD_CtlError(pdev, req);
				return USBD_FAIL;
			}

			/* DFU interface requests */
			if (req->wIndex == USB_CDC_UART_INTERFACE_DFU) {
				return USBD_CDC_UART_DFU_Request(pdev, req);
			}

			switch (req->bRequest) {
				case CDC_REQ_SET_LINE_CODING:
					hcdc->last_setup_request = *req;
					USBD_CtlPrepareRx(pdev, hcdc->ep0_buf, 7);
					return USBD_OK;
				case CDC_REQ_GET_LINE_CODING: {
					hcdc->ep0_buf[0] = (uint8_t)(hcdc->dwDTERate & 0xFF);
					hcdc->ep0_buf[1] = (uint8_t)((hcdc->dwDTERate >> 8) & 0xFF);
					hcdc->ep0_buf[2] = (uint8_t)((hcdc->dwDTERate >> 16) & 0xFF);
					hcdc->ep0_buf[3] = (uint8_t)((hcdc->dwDTERate >> 24) & 0xFF);
					hcdc->ep0_buf[4] = hcdc->bCharFormat;
					hcdc->ep0_buf[5] = hcdc->bParityType;
					hcdc->ep0_buf[6] = hcdc->bDataBits;
					USBD_CtlSendData(pdev, hcdc->ep0_buf, 7);
					return USBD_OK;
				}
				case CDC_REQ_SET_CONTROL_LINE_STATE:
					/* Ignore, but ACK. */
					USBD_CtlSendStatus(pdev);
					return USBD_OK;
				case CDC_REQ_SEND_BREAK:
					USBD_CtlSendStatus(pdev);
					return USBD_OK;
				case CDC_REQ_SEND_ENCAPSULATED_COMMAND:
				case CDC_REQ_GET_ENCAPSULATED_RESPONSE:
				default:
					USBD_CtlError(pdev, req);
					return USBD_FAIL;
			}
		}
		default:
			break;
	}

	return USBD_OK;
}

static uint8_t USBD_CDC_UART_EP0_RxReady(USBD_HandleTypeDef *pdev)
{
	USBD_CDC_UART_HandleTypeDef *hcdc = cdc_handle(pdev);
	USBD_SetupReqTypedef *req = &hcdc->last_setup_request;

	if ((req->bmRequest & USB_REQ_TYPE_MASK) == USB_REQ_TYPE_CLASS &&
		req->bRequest == CDC_REQ_SET_LINE_CODING) {

		hcdc->dwDTERate = (uint32_t)hcdc->ep0_buf[0] |
			((uint32_t)hcdc->ep0_buf[1] << 8) |
			((uint32_t)hcdc->ep0_buf[2] << 16) |
			((uint32_t)hcdc->ep0_buf[3] << 24);
		hcdc->bCharFormat = hcdc->ep0_buf[4];
		hcdc->bParityType = hcdc->ep0_buf[5];
		hcdc->bDataBits = hcdc->ep0_buf[6];

		USBD_CDC_UART_ApplyLineCoding(hcdc);
	}

	return USBD_OK;
}

static uint8_t USBD_CDC_UART_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
	USBD_CDC_UART_HandleTypeDef *hcdc = cdc_handle(pdev);

	if (epnum == (USB_CDC_UART_EP_DATA_OUT & 0x7F)) {
		uint32_t rx_len = USBD_LL_GetRxDataSize(pdev, epnum);
		if (rx_len > 0) {
			(void)uart_bridge_write(hcdc->rx_buf, (size_t)rx_len);
		}
		USBD_LL_PrepareReceive(pdev, USB_CDC_UART_EP_DATA_OUT, hcdc->rx_buf, USB_CDC_UART_DATA_MAX_PACKET_SIZE);
	}

	return USBD_OK;
}

static uint8_t USBD_CDC_UART_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
	USBD_CDC_UART_HandleTypeDef *hcdc = cdc_handle(pdev);

	if (epnum == (USB_CDC_UART_EP_DATA_IN & 0x7F)) {
		hcdc->tx_busy = false;
	}

	return USBD_OK;
}

static uint8_t *USBD_CDC_UART_GetCfgDesc(uint16_t *length)
{
	*length = (uint16_t)sizeof(USBD_CDC_UART_CfgDesc);
	memcpy(USBD_DescBuf, USBD_CDC_UART_CfgDesc, sizeof(USBD_CDC_UART_CfgDesc));
	return USBD_DescBuf;
}

static uint8_t *USBD_CDC_UART_GetStrDesc(USBD_HandleTypeDef *pdev, uint8_t index, uint16_t *length)
{
	UNUSED(pdev);

	switch (index) {
		case DFU_INTERFACE_STR_INDEX:
			USBD_GetString((uint8_t *)DFU_INTERFACE_STRING_FS, USBD_DescBuf, length);
			return USBD_DescBuf;
		default:
			*length = 0;
			return NULL;
	}
}

void USBD_CDC_UART_Task(USBD_CDC_UART_HandleTypeDef *hcdc)
{
	if (!hcdc || !hcdc->usb) {
		return;
	}

	uart_bridge_poll();

	if (hcdc->tx_busy) {
		return;
	}

	uint16_t to_send = (uint16_t)uart_bridge_read(hcdc->tx_buf, sizeof(hcdc->tx_buf));
	if (to_send == 0) {
		return;
	}

	hcdc->tx_busy = true;
	(void)USBD_LL_Transmit(hcdc->usb, USB_CDC_UART_EP_DATA_IN, hcdc->tx_buf, to_send);
}

bool USBD_CDC_UART_DfuDetachRequested(USBD_HandleTypeDef *pdev)
{
	USBD_CDC_UART_HandleTypeDef *hcdc = cdc_handle(pdev);
	return hcdc ? hcdc->dfu_detach_requested : false;
}

/* No custom MS OS / WINUSB descriptors needed for CDC mode. */
bool USBD_CDC_UART_CustomDeviceRequest(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
	UNUSED(pdev);
	UNUSED(req);
	return false;
}

bool USBD_CDC_UART_CustomInterfaceRequest(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
	UNUSED(pdev);
	UNUSED(req);
	return false;
}

void USBD_CDC_UART_SuspendCallback(USBD_HandleTypeDef *pdev)
{
	UNUSED(pdev);
}

void USBD_CDC_UART_ResumeCallback(USBD_HandleTypeDef *pdev)
{
	UNUSED(pdev);
}
