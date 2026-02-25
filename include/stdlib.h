/* Minimal stdlib.h for freestanding build */
#ifndef CANDLE_STDLIB_H
#define CANDLE_STDLIB_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Minimal definitions used by the firmware */
void _exit(int status);

#ifndef NULL
#define NULL ((void*)0)
#endif

#ifdef __cplusplus
}
#endif
#endif
