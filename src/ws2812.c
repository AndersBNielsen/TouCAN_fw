#include <stddef.h>
#include <stdint.h>

#include "config.h"
#include "hal_include.h"
#include "util_macro.h"
#include "ws2812.h"

#if IS_ENABLED(CONFIG_WS2812_STARTUP)

#ifndef WS2812_GPIO_Port
#error "WS2812_GPIO_Port must be defined when CONFIG_WS2812_STARTUP is enabled"
#endif
#ifndef WS2812_Pin
#error "WS2812_Pin must be defined when CONFIG_WS2812_STARTUP is enabled"
#endif
#ifndef WS2812_LED_COUNT
#error "WS2812_LED_COUNT must be defined when CONFIG_WS2812_STARTUP is enabled"
#endif

/*
 * WS2812 timing at 800kHz, using a free-running 48MHz timer (TIM14).
 * One bit period is 1.25us => 60 ticks at 48MHz.
 */
#define WS2812_TICKS_PER_BIT 60u
#define WS2812_T0H_TICKS     17u
#define WS2812_T1H_TICKS     34u

static void ws2812_gpio_init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	/* gpio clocks are enabled in gpio_init(), but this keeps the driver standalone */
	if (WS2812_GPIO_Port == GPIOA) {
		__HAL_RCC_GPIOA_CLK_ENABLE();
	} else if (WS2812_GPIO_Port == GPIOB) {
		__HAL_RCC_GPIOB_CLK_ENABLE();
	} else if (WS2812_GPIO_Port == GPIOC) {
		__HAL_RCC_GPIOC_CLK_ENABLE();
	}

	HAL_GPIO_WritePin(WS2812_GPIO_Port, WS2812_Pin, GPIO_PIN_RESET);

	GPIO_InitStruct.Pin = WS2812_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.Alternate = 0;
	HAL_GPIO_Init(WS2812_GPIO_Port, &GPIO_InitStruct);
}

static void ws2812_tim14_init(void)
{
	__HAL_RCC_TIM14_CLK_ENABLE();

	TIM14->CR1 = 0;
	TIM14->CR2 = 0;
	TIM14->DIER = 0;
	TIM14->PSC = 0;
	TIM14->ARR = 0xFFFF;
	TIM14->EGR = TIM_EGR_UG;
	TIM14->CR1 |= TIM_CR1_CEN;
}

static inline uint16_t ws2812_tim14_cnt(void)
{
	return (uint16_t)TIM14->CNT;
}

static inline void ws2812_wait_ticks(uint16_t start, uint16_t ticks)
{
	while ((uint16_t)(ws2812_tim14_cnt() - start) < ticks) {
		/* busy wait */
	}
}

static void ws2812_write_grb(const uint8_t *grb, size_t len)
{
	const uint32_t set_mask = (uint32_t)WS2812_Pin;
	const uint32_t reset_mask = (uint32_t)WS2812_Pin << 16;

	uint32_t primask = __get_PRIMASK();
	__disable_irq();

	for (size_t i = 0; i < len; i++) {
		uint8_t byte = grb[i];
		for (unsigned bit = 0; bit < 8; bit++) {
			const uint16_t start = ws2812_tim14_cnt();
			const uint16_t t_high = (byte & 0x80u) ? WS2812_T1H_TICKS : WS2812_T0H_TICKS;

			WS2812_GPIO_Port->BSRR = set_mask;
			ws2812_wait_ticks(start, t_high);
			WS2812_GPIO_Port->BSRR = reset_mask;
			ws2812_wait_ticks(start, WS2812_TICKS_PER_BIT);

			byte <<= 1;
		}
	}

	if (!primask) {
		__enable_irq();
	}

	/* Reset latch: keep low for >50us. 1ms is cheap at boot and safe. */
	HAL_Delay(1);
}

static void wheel_rgb(uint8_t pos, uint8_t brightness, uint8_t *r, uint8_t *g, uint8_t *b)
{
	uint8_t rr = 0, gg = 0, bb = 0;

	/* 3-segment rainbow wheel, 0..255 */
	if (pos < 85u) {
		rr = (uint8_t)(255u - (uint16_t)pos * 3u);
		gg = (uint8_t)((uint16_t)pos * 3u);
		bb = 0;
	} else if (pos < 170u) {
		pos = (uint8_t)(pos - 85u);
		rr = 0;
		gg = (uint8_t)(255u - (uint16_t)pos * 3u);
		bb = (uint8_t)((uint16_t)pos * 3u);
	} else {
		pos = (uint8_t)(pos - 170u);
		rr = (uint8_t)((uint16_t)pos * 3u);
		gg = 0;
		bb = (uint8_t)(255u - (uint16_t)pos * 3u);
	}

	/* scale brightness (0..255) */
	*r = (uint8_t)(((uint16_t)rr * brightness) / 255u);
	*g = (uint8_t)(((uint16_t)gg * brightness) / 255u);
	*b = (uint8_t)(((uint16_t)bb * brightness) / 255u);
}

void ws2812_startup_rainbow(void)
{
	ws2812_gpio_init();
	ws2812_tim14_init();

	uint8_t grb[WS2812_LED_COUNT * 3];

	/* One static rainbow frame across the strip. */
	for (uint32_t i = 0; i < WS2812_LED_COUNT; i++) {
		uint8_t r, g, b;
		uint8_t hue = (uint8_t)((i * 256u) / WS2812_LED_COUNT);
		wheel_rgb(hue, 64u, &r, &g, &b);

		grb[i * 3u + 0u] = g;
		grb[i * 3u + 1u] = r;
		grb[i * 3u + 2u] = b;
	}

	ws2812_write_grb(grb, sizeof(grb));
}

#else

void ws2812_startup_rainbow(void)
{
	/* disabled */
}

#endif
