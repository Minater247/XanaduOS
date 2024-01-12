#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "inc_c/display.h"
#include "inc_c/boot.h"
#include "include/filesystem.h"
#include "inc_c/memory.h"
#include "inc_c/elf_x86.h"

//temp
#include "../arch/x86/drivers/keyboard.h"


void kernel_main() {
	boot_initialize();


	//attempt to load hello.elf
	file_descriptor_t fd = fopen("/mnt/ramdisk/bin/hello.elf", 0);
	if (fd.flags & FILE_NOTFOUND_FLAG || !(fd.flags & FILE_ISOPEN_FLAG))
	{
		terminal_printf("Could not locate hello.elf!\n");
		while (true);
	}
	int size = fgetsize(&fd);
	terminal_printf("Size of hello.elf: %d\n", size);
	char *elfbuf = kmalloc(size);
	int read = fread(elfbuf, 1, size, &fd);
	if (read != size)
	{
		terminal_printf("Could not read hello.elf!\n");
		while (true);
	}
	terminal_printf("Read %d bytes from hello.elf\n", read);
	fclose(&fd);
	
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
	terminal_printf("Returned: %d\n", ret);

	//test WRITE syscall on "Hello, world!"
	terminal_printf("Testing WRITE syscall...\n");
	//we have to use inline assembly since syscall_write hasn't been implemented yet
	char *str = "Hello, world!\n";
	asm volatile (
		"movl $1, %%eax;"      // syscall number (sys_write)
		"movl $1, %%edi;"      // file descriptor (stdout)
		"movl %0, %%esi;"      // buffer to write from
		"movl $13, %%edx;"     // number of bytes
		"int $0x80;"           // call kernel
		:
		: "r" (str)
		: "eax", "ebx", "ecx", "edx"
	);


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