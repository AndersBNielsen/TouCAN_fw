/* Minimal string.h for freestanding build */
#ifndef CANDLE_STRING_H
#define CANDLE_STRING_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);
size_t strlen(const char *s);
void *memmove(void *dest, const void *src, size_t n);
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t n);

#ifdef __cplusplus
}
#endif
#endif
