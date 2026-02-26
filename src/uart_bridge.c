#include "uart_bridge.h"

#include <string.h>

#include "hal_include.h"

/* Optional hooks for activity indication (e.g. blink LEDs).
 * Implement these elsewhere (non-weak) to handle RX/TX events.
 * Must be ISR-safe.
 */
void __attribute__((weak)) uart_bridge_on_rx_activity(void)
{
}

void __attribute__((weak)) uart_bridge_on_tx_activity(void)
{
}

#if defined(STM32F0) && defined(USART1)

#ifndef UART_BRIDGE_RX_BUF_SIZE
#define UART_BRIDGE_RX_BUF_SIZE 256u
#endif
#ifndef UART_BRIDGE_TX_BUF_SIZE
#define UART_BRIDGE_TX_BUF_SIZE 256u
#endif

static volatile uint8_t rx_buf[UART_BRIDGE_RX_BUF_SIZE];
static volatile uint16_t rx_head;
static volatile uint16_t rx_tail;

static volatile uint8_t tx_buf[UART_BRIDGE_TX_BUF_SIZE];
static volatile uint16_t tx_head;
static volatile uint16_t tx_tail;

static bool usart_ready;

static uint16_t rb_next(uint16_t cur, uint16_t size)
{
	uint16_t next = (uint16_t)(cur + 1u);
	return (next >= size) ? 0u : next;
}

static size_t rb_count(uint16_t head, uint16_t tail, uint16_t size)
{
	if (head >= tail) {
		return (size_t)(head - tail);
	}
	return (size_t)(size - (tail - head));
}

static void usart_enable_txe_irq_if_needed(void)
{
	if (tx_head != tx_tail) {
		USART1->CR1 |= USART_CR1_TXEIE;
	}
}

static void usart_apply_baud(uint32_t baudrate)
{
	/* On STM32F0, HAL only exposes PCLK1. For typical clock trees on these
	 * parts, PCLK1 matches the USART kernel clock used here.
	 */
	uint32_t pclk = HAL_RCC_GetPCLK1Freq();
	if (pclk == 0u || baudrate == 0u) {
		return;
	}

	/* Oversampling by 16. BRR = fCK / baud.
	 * Rounded to nearest.
	 */
	uint32_t brr = (pclk + (baudrate / 2u)) / baudrate;
	USART1->BRR = brr;
}

static void usart_config_pins(void)
{
	__HAL_RCC_GPIOB_CLK_ENABLE();

	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF0_USART1;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void uart_bridge_init(const uart_line_coding_t *coding)
{
	rx_head = rx_tail = 0;
	tx_head = tx_tail = 0;
	usart_ready = false;

	__HAL_RCC_USART1_CLK_ENABLE();
	usart_config_pins();

	HAL_NVIC_SetPriority(USART1_IRQn, 2, 0);
	HAL_NVIC_EnableIRQ(USART1_IRQn);

	/* Disable USART before reconfig */
	USART1->CR1 &= ~USART_CR1_UE;

	/* Basic 8N1, no flow control. We only reliably apply baudrate here.
	 * (CDC line coding parity/stopbits can be extended if needed.)
	 */
	USART1->CR2 = 0;
	USART1->CR3 = 0;

	uint32_t baud = 115200u;
	if (coding && coding->baudrate) {
		baud = coding->baudrate;
	}
	usart_apply_baud(baud);

	/* Enable RX, TX and RXNE interrupt */
	USART1->CR1 = USART_CR1_RE | USART_CR1_TE | USART_CR1_RXNEIE;
	USART1->CR1 |= USART_CR1_UE;

	usart_ready = true;
}

void uart_bridge_set_line_coding(const uart_line_coding_t *coding)
{
	if (!usart_ready) {
		uart_bridge_init(coding);
		return;
	}

	uint32_t baud = 115200u;
	if (coding && coding->baudrate) {
		baud = coding->baudrate;
	}

	USART1->CR1 &= ~USART_CR1_UE;
	usart_apply_baud(baud);
	USART1->CR1 |= USART_CR1_UE;
}

bool uart_bridge_is_ready(void)
{
	return usart_ready;
}

size_t uart_bridge_write(const uint8_t *data, size_t len)
{
	if (!usart_ready || !data || len == 0) {
		return 0;
	}

	size_t written = 0;
	for (; written < len; written++) {
		uint16_t next = rb_next(tx_head, UART_BRIDGE_TX_BUF_SIZE);
		if (next == tx_tail) {
			break;
		}
		tx_buf[tx_head] = data[written];
		tx_head = next;
	}

	usart_enable_txe_irq_if_needed();
	return written;
}

size_t uart_bridge_read(uint8_t *data, size_t max_len)
{
	if (!usart_ready || !data || max_len == 0) {
		return 0;
	}

	size_t available = rb_count(rx_head, rx_tail, UART_BRIDGE_RX_BUF_SIZE);
	if (available == 0) {
		return 0;
	}

	size_t to_read = (available < max_len) ? available : max_len;
	for (size_t i = 0; i < to_read; i++) {
		data[i] = rx_buf[rx_tail];
		rx_tail = rb_next(rx_tail, UART_BRIDGE_RX_BUF_SIZE);
	}

	return to_read;
}

void uart_bridge_poll(void)
{
	usart_enable_txe_irq_if_needed();
}

void USART1_IRQHandler(void)
{
	uint32_t isr = USART1->ISR;

	if (isr & USART_ISR_RXNE) {
		uint8_t b = (uint8_t)USART1->RDR;
		uint16_t next = rb_next(rx_head, UART_BRIDGE_RX_BUF_SIZE);
		if (next != rx_tail) {
			rx_buf[rx_head] = b;
			rx_head = next;
		}
		uart_bridge_on_rx_activity();
	}

	if ((USART1->CR1 & USART_CR1_TXEIE) && (isr & USART_ISR_TXE)) {
		if (tx_head == tx_tail) {
			USART1->CR1 &= ~USART_CR1_TXEIE;
		} else {
			USART1->TDR = tx_buf[tx_tail];
			tx_tail = rb_next(tx_tail, UART_BRIDGE_TX_BUF_SIZE);
			uart_bridge_on_tx_activity();
		}
	}

	/* Clear error flags by writing to ICR. */
	if (isr & (USART_ISR_ORE | USART_ISR_FE | USART_ISR_NE | USART_ISR_PE)) {
		uint32_t icr = USART_ICR_ORECF | USART_ICR_FECF | USART_ICR_PECF;
#if defined(USART_ICR_NCF)
		icr |= USART_ICR_NCF;
#elif defined(USART_ICR_NECF)
		icr |= USART_ICR_NECF;
#endif
		USART1->ICR = icr;
	}
}

#else

void uart_bridge_init(const uart_line_coding_t *coding)
{
	(void)coding;
}

void uart_bridge_set_line_coding(const uart_line_coding_t *coding)
{
	(void)coding;
}

size_t uart_bridge_write(const uint8_t *data, size_t len)
{
	(void)data;
	(void)len;
	return 0;
}

size_t uart_bridge_read(uint8_t *data, size_t max_len)
{
	(void)data;
	(void)max_len;
	return 0;
}

void uart_bridge_poll(void)
{
}

bool uart_bridge_is_ready(void)
{
	return false;
}

#endif
