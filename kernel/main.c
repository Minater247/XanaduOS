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

	terminal_printf("Valid file!");

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
	fclose(&fd);

	//set color to white
	terminal_printf("\n\033[0m");
	dir_descriptor_t dd = fopendir("/mnt/ramdisk/", 0);
	if (dd.flags & FILE_NOTFOUND_FLAG || !(dd.flags & FILE_ISOPENDIR_FLAG))
	{
		terminal_printf("Directory not found!\n");
		while (true);
	}

	//print the contents
	terminal_printf("Valid directory!\n");
	terminal_printf("Contents:\n");
	simple_return_t ret = freaddir(&dd);
	while (!(ret.flags & FILE_NOTFOUND_FLAG))
	{
		terminal_printf(ret.name);
		terminal_printf("\n");
		ret = freaddir(&dd);
	}
	fclosedir(&dd);





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