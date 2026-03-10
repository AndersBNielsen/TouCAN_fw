#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/*
 * WS2812B debug output.
 *
 * This is intentionally minimal: a blocking one-shot rainbow frame sent at boot
 * when CONFIG_WS2812_STARTUP is enabled.
 */
void ws2812_startup_rainbow(void);

#ifdef __cplusplus
}
#endif
