#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <kernel/tty.h>
#include <kernel/asmfuncs.h>


void enable_cursor(uint8_t cursor_start, uint8_t cursor_end)
{
	outb(0x3D4, 0x0A);
	outb(0x3D5, (inb(0x3D5) & 0xC0) | cursor_start);
 
	outb(0x3D4, 0x0B);
	outb(0x3D5, (inb(0x3D5) & 0xE0) | cursor_end);
}

void disable_cursor()
{
	outb(0x3D4, 0x0A);
	outb(0x3D5, 0x20);
}

void kernel_main(void) {
	terminal_initialize();
	disable_cursor();
	for (int i=0; i < 30; i++) {
		printf("Line ");
		char int_str[15];
		itoa(i, int_str, 10);
		printf(int_str);
		printf("\n");
	}
}