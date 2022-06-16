#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <kernel/tty.h>
#include <kernel/asmfuncs.h>

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