#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "inc_c/display.h"
#include "inc_c/boot.h"

 
void kernel_main(void) 
{
	boot_initialize();

	terminal_initialize();
	terminal_writestring("Hello, kernel World!\n");
	terminal_writestring("This is a new line!\n");

	while (true);
}