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
	if (long_mode_available()) {
		printf("Long mode is available.\n");
	} else {
		printf("Long mode is not available.\n");
	}
	//check a20 gate
	if (check_a20_gate()) {
		printf("A20 gate is enabled.\n");
	} else {
		printf("A20 gate is not enabled.\n");
	}
}