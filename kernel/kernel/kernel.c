#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <kernel/tty.h>
#include <kernel/asmfuncs.h>

uint32_t page_directory[1024] __attribute__((aligned(4096)));
uint32_t basic_page_table[1024] __attribute__((aligned(4096)));

extern void check_a20();
extern int a20_return;

void kernel_main(void) {
	terminal_initialize();
	int CPUID_available = check_CPUID();
	if (CPUID_available) {
		printf("CPUID is available.\n");
	} else {
		printf("CPUID is not available.\n");
	}
	char vendor[13];
	get_processor_vendor(vendor);
	printf("Processor vendor: %s\n", vendor);

	check_a20();
}