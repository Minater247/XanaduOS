#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "inc_c/display.h"
#include "inc_c/boot.h"

//temp
#include "../arch/x86/drivers/keyboard.h"


void kernel_main() {
	boot_initialize();

	char buf[2] = {0, 0};
	while (true) {
		char code = keyboard_getchar();
		if (code != 0)
		{
			if (code == '\n')
			{
				terminal_writestring("\n");
			} else {
				buf[0] = code;
				terminal_writestring(buf);
			}
		}
	}

	while (true);
}