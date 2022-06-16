#include <stdio.h>
#include <string.h>

#include <kernel/tty.h>

void kernel_main(void) {
	terminal_initialize();
	for (int i=0; i < 30; i++) {
		printf("Line ");
		char int_str[15];
		itoa(i, int_str, 10);
		printf(int_str);
		printf("\n");
	}
}
