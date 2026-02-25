/* Minimal implementations of a few libc functions used in the firmware.
   Keep implementations small and portable. */
#include <stddef.h>

void *memcpy(void *dest, const void *src, size_t n)
{
    unsigned char *d = (unsigned char*)dest;
    const unsigned char *s = (const unsigned char*)src;
    while (n--) *d++ = *s++;
    return dest;
}

void *memset(void *s, int c, size_t n)
{
    unsigned char *p = (unsigned char*)s;
    while (n--) *p++ = (unsigned char)c;
    return s;
}

int memcmp(const void *s1, const void *s2, size_t n)
{
    const unsigned char *a = (const unsigned char*)s1;
    const unsigned char *b = (const unsigned char*)s2;
    for (size_t i = 0; i < n; ++i) {
        if (a[i] != b[i]) return (int)a[i] - (int)b[i];
    }
    return 0;
}

size_t strlen(const char *s)
{
    const char *p = s;
    while (*p) ++p;
    return (size_t)(p - s);
}

void *memmove(void *dest, const void *src, size_t n)
{
    unsigned char *d = (unsigned char*)dest;
    const unsigned char *s = (const unsigned char*)src;
    if (d < s) {
        while (n--) *d++ = *s++;
    } else {
        d += n; s += n;
        while (n--) *--d = *--s;
    }
    return dest;
}

char *strcpy(char *dest, const char *src)
{
    char *d = dest;
    while ((*d++ = *src++));
    return dest;
}

char *strncpy(char *dest, const char *src, size_t n)
{
    char *d = dest;
    size_t i = 0;
    for (; i < n && src[i]; ++i) d[i] = src[i];
    for (; i < n; ++i) d[i] = '\0';
    return dest;
}

/* Provide common ARM EABI aliases that toolchains may emit */
void * __aeabi_memcpy(void *d, const void *s, size_t n) __attribute__((weak, alias("memcpy")));
void * __aeabi_memmove(void *d, const void *s, size_t n) __attribute__((weak, alias("memmove")));
void * __aeabi_memset(void *s, int c, size_t n) __attribute__((weak, alias("memset")));
