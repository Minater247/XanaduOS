#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "inc_c/display.h"
#include "inc_c/boot.h"
#include "include/filesystem.h"
#include "inc_c/memory.h"
#include "inc_c/arch_elf.h"
#include "inc_c/process.h"
#include "include/errors.h"
#include "inc_c/serial.h"

//temp
#include "../arch/x86/drivers/keyboard.h"


void print_stuff() {
	while (true) {
		terminal_printf("A");
	}
}

void print_more() {
	while (true) {
		terminal_printf("C");
	}
}

int some_process() {
	terminal_printf("Hello from some_process!\n");
	return 0xBEAA;
}


void kernel_main() {
	asm volatile ("cli");

	boot_initialize();

	fopen("/dev/kbd0", "r"); // stdin
	fopen("/dev/trm", "r+"); // stdout
	fopen("/dev/trm", "r+"); // stderr

	asm volatile ("sti");

	uint32_t pid = fork();

	if (pid == 0) {
		terminal_printf("Hello from child!\n");
		while (true);
	} else {
		terminal_printf("Hello from parent!\n");
	}

	process_t *new_process = process_load_elf("/mnt/ramdisk/bin/xansh.elf");
	terminal_printf("Process loaded with PID %d\n", new_process->pid);

	while (new_process->status != TASK_STATUS_FINISHED) {}
	terminal_printf("\nProcess finished with code 0x%x\n", new_process->entry_or_return);

	terminal_printf("Kernel is finished running. Press q to page fault!\n");

    char buf;
	file_descriptor_t *kbd = stdin;
	while (true) {
		int read = fread(&buf, 1, 1, kbd);
		if (read != 0) {
			if (buf == 'q') {
				terminal_printf("%c", *(char *)0xA0000000);
			}
		}
	}
}