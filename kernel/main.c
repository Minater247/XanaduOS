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


void kernel_main() {
	boot_initialize();

	fopen("/dev/kbd0", 0); // stdin
	fopen("/dev/trm", 0); // stdout
	fopen("/dev/trm", 0); // stderr

	//int code = process_load_elf("/mnt/ramdisk/bin/hello.elf");
	//terminal_printf("Loaded ELF with PID %d\n", code);

	create_task(&print_stuff, kmalloc(4096));
	create_task(&print_more, kmalloc(4096));

	while (true) {
		terminal_printf("B");
	}

    char buf;
	file_descriptor_t *kbd = stdin;
	while (true) {
		int read = fread(&buf, 1, 1, kbd);
		if (read != 0)
		{
			terminal_putchar(buf);
		}
	}

	while (true);
}