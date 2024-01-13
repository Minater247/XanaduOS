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


void kernel_main() {
	boot_initialize();

	fopen("/dev/kbd0", 0); // stdin
	fopen("/dev/trm", 0); // stdout
	fopen("/dev/trm", 0); // stderr


	//attempt to load hello.elf
	file_descriptor_t *fd = fopen("/mnt/ramdisk/bin/hello.elf", 0);
	if (fd->flags & FILE_NOTFOUND_FLAG || !(fd->flags & FILE_ISOPEN_FLAG))
	{
		terminal_printf("Could not locate hello.elf!\n");
		while (true);
	}

	stat_t stat;
	int stat_ret = fstat(fd, &stat);
	if (stat_ret != 0)
	{
		terminal_printf("Could not stat hello.elf!\n");
		while (true);
	}
	int size = stat.st_size;

	terminal_printf("Size of hello.elf: %d\n", size);
	char *elfbuf = kmalloc(size);
	int read = fread(elfbuf, 1, size, fd);
	if (read != size)
	{
		terminal_printf("Could not read hello.elf!\n");
		while (true);
	}
	terminal_printf("Read %d bytes from hello.elf\n", read);

	fclose(fd);
	
	elf_load_result_t loaded = elf_load_executable(elfbuf);
	if (loaded.code != 0)
	{
		terminal_printf("Could not load hello.elf! Error code: %d\n", loaded.code);
		while (true);
	}
	kfree(elfbuf);

	//print success in green
	terminal_printf("\x1b[32mSuccessfully loaded hello.elf!\x1b[0m\n");
	terminal_printf("Entry point: 0x%x\n", loaded.entry_point);
	//jump to entry point - we expect an int to be returned
	int (*entry_point)() = (int (*)())loaded.entry_point;
	int ret = entry_point();
	terminal_printf("\n\033[0mReturned: %d\n", ret);

	
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