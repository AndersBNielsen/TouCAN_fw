/* Minimal runtime stubs for freestanding build */
#include <stdint.h>

/* Provide the division helpers that other objects may reference. */
__attribute__((weak)) unsigned int __aeabi_uidiv(unsigned int a, unsigned int b)
{
    return b ? (a / b) : 0;
}

__attribute__((weak)) int __aeabi_idiv(int a, int b)
{
    return b ? (a / b) : 0;
}

__attribute__((weak)) unsigned long __aeabi_uldivmod(unsigned long a, unsigned long b)
{
    return b ? (a / b) : 0UL;
}

__attribute__((weak)) unsigned long __aeabi_uidivmod(unsigned long a, unsigned long b)
{
    return b ? (a / b) : 0UL;
}
