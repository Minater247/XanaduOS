#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include <kernel/tty.h>
#include <kernel/asmfuncs.h>
#include <kernel/GDT.h>
#include <kernel/IDT.h>

uint32_t page_directory[1024] __attribute__((aligned(4096)));
uint32_t basic_page_table[1024] __attribute__((aligned(4096)));

int check_A20();
int checkProtected();

void kernel_main(void) {
	terminal_initialize();

	if (checkProtected()) {
		terminal_writestring("Protected Mode is enabled.\n");
	} else {
		terminal_writestring("Protected Mode is disabled.\n");
	}

	int CPUID_available = check_CPUID();
	if (CPUID_available) {
		printf("CPUID is available.\n");
	} else {
		printf("CPUID is not available.\n");
	}
	char vendor[13];
	get_processor_vendor(vendor);
	printf("Processor vendor: %s\n", vendor);

	gdt_install();
	printf("GDT loaded.\n");

	printf("Loading IDT...\n");
	idt_install();
	printf("IDT loaded.\n");
	idt_enable(true);
	printf("IDT enabled.\n");

	if (check_A20()) {
		printf("A20 is enabled.\n");
	} else {
		printf("A20 is disabled.\n");
	}
}