#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct _copy_table_t
{
	uint32_t const* src;
	uint32_t* dest;
	uint32_t wlen;
} copy_table_t;

extern const copy_table_t __copy_table_start__;
extern const copy_table_t __copy_table_end__;

extern uint32_t __bss_start__;
extern uint32_t __bss_end__;

void __initialize_hardware_early(void);
int main(void);
void _exit(int code);

void Reset_Handler(void)
{
	__initialize_hardware_early();

	for (copy_table_t const* table = &__copy_table_start__; table < &__copy_table_end__; ++table) {
		memcpy(table->dest, table->src, table->wlen);
	}

	for (uint32_t *p = &__bss_start__; p < &__bss_end__; ++p) {
		*p = 0;
	}

	(void)main();
	_exit(0);
}

void __register_exitproc(void) {
	return;
}

void _exit(int code)
{
	(void) code;
	__asm__ ("BKPT");
	while (1);
}
