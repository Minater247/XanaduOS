#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include <kernel/tty.h>
#include <kernel/asmfuncs.h>
#include <kernel/GDT.h>
#include <kernel/IDT.h>
#include <kernel/IRQ.h>
#include <kernel/keyboard.h>
#include <kernel/PIC.h>

uint32_t page_directory[1024] __attribute__((aligned(4096)));
uint32_t basic_page_table[1024] __attribute__((aligned(4096)));

int check_A20();
int checkProtected();
void a20_bios();
void a20_in();

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

	printf("Setting up basic IRQs...\n");
	keyboard_install();
	printf("IRQs created.\n");

	printf("Loading IDT...\n");
	idt_install();
	printf("IDT loaded.\n");
	idt_enable(true);
	printf("IDT enabled.\n");

	if (check_A20()) {
		printf("A20 is enabled. No action taken.\n");
	} else {
		printf("A20 is disabled.\n");
		printf("Attempting to enable via BIOS...\n");
		a20_bios();
		if (!check_A20()) {
			printf("A20 is still disabled.\n");
			printf("Attempting to enable via IN 0xee");
			a20_in();
			if (!check_A20()) {
				printf("Unable to enable A20 line.\n");
			} else {
				printf("A20 is now enabled.\n");
			}
		}
		else {
			printf("A20 successfully enabled.\n");
		}
	}

	keyboard_install();
	clear_irq_mask(2);
	clear_irq_mask(1);
}