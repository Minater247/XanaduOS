#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "inc_c/display.h"
#include "inc_c/boot.h"

//temp
#include "../arch/x86/drivers/keyboard.h"

 
void kernel_main(void) 
{
	boot_initialize();

	terminal_initialize();
	terminal_writestring("Hello, kernel World!\n");
	terminal_writestring("This is a new line!\n");

	while (true) {
		char code = keyboard_getchar();
		if (code != 0)
		{
			if (code == '\n')
			{
				terminal_writestring("\n");
			} else {
				terminal_putchar(code);
			}
		}
	}

	while (true);
}