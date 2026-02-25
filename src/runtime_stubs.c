/* Minimal runtime stubs for freestanding build */
#include <stdint.h>

/* Provided by CMSIS / HAL sources */
extern void SystemInit(void);

/* user code */
int main(void);
void _exit(int code);

/* Entry point called from Reset_Handler — call main().
   SystemInit() is called earlier in Reset_Handler via
   __initialize_hardware_early(), so avoid double-initialization here. */
void _start(void)
{
    (void) main();
    _exit(0);
    for (;;) ;
}

/* Provide the division helpers that other objects may reference. */
unsigned int __aeabi_uidiv(unsigned int a, unsigned int b)
{
    return b ? (a / b) : 0;
}

int __aeabi_idiv(int a, int b)
{
    return b ? (a / b) : 0;
}

unsigned long __aeabi_uldivmod(unsigned long a, unsigned long b)
{
    return b ? (a / b) : 0UL;
}

unsigned long __aeabi_uidivmod(unsigned long a, unsigned long b)
{
    return b ? (a / b) : 0UL;
}
