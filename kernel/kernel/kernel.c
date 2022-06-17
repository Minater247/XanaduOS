#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <kernel/tty.h>
#include <kernel/asmfuncs.h>

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
}