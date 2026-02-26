#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
	uint32_t baudrate;
	uint8_t data_bits;
	uint8_t stop_bits; /* 1 or 2 */
	uint8_t parity;    /* 0:none, 1:odd, 2:even */
} uart_line_coding_t;

void uart_bridge_init(const uart_line_coding_t *coding);
void uart_bridge_set_line_coding(const uart_line_coding_t *coding);

/* Host->UART */
size_t uart_bridge_write(const uint8_t *data, size_t len);

/* UART->Host */
size_t uart_bridge_read(uint8_t *data, size_t max_len);

void uart_bridge_poll(void);

bool uart_bridge_is_ready(void);
