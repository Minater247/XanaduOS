#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "inc_c/display.h"
#include "inc_c/boot.h"
#include "include/filesystem.h"

//temp
#include "../arch/x86/drivers/keyboard.h"


void kernel_main() {
	boot_initialize();

	file_descriptor_t fd = fopen("/mnt/ramdisk/logo.txt", 0);
	char buf2[101];
	//check the flags
	if (fd.flags & FILE_NOTFOUND_FLAG || !(fd.flags & FILE_ISOPEN_FLAG))
	{
		terminal_printf("File not found!\n");
		while (true);
	}

	//set color to lime
	terminal_printf("\033[32m");
	int read = fread(buf2, 1, 100, &fd);
	while (read > 0)
	{
		for (int i = 0; i < read; i++)
		{
			terminal_putchar(buf2[i]);
		}
		read = fread(buf2, 1, 100, &fd);
	}

	char buf[2] = {0, 0};
	while (true) {
		char code = keyboard_getchar();
		if (code != 0)
		{
			if (code == '\n')
			{
				terminal_printf("\n");
			} else {
				buf[0] = code;
				terminal_printf(buf);
			}
		}
	}

	while (true);
}