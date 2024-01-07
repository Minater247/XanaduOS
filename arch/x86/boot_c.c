#include <stdint.h>

#include "inc_c/boot.h"
#include "inc_c/display.h"
#include "inc_c/tables.h"
#include "inc_c/hardware.h"
#include "inc_c/multiboot.h"
#include "../../kernel/include/errors.h"

extern uint32_t given_magic;
extern uint32_t given_mboot;

// This is the first function called by the kernel.
// This function should:
// - Initialize all processor systems (GDT, IDT, PIC, etc.)
// - Initialize all kernel systems (terminal, memory, etc.)
// - Initialize all drivers (keyboard, mouse, etc.)
// - Initialize all kernel modules (filesystem, etc.)
// - Initialize multitasking components (processes, threads, etc.) if available
void boot_initialize() {
    uint32_t magic = given_magic;
    uint32_t mboot_ptr = given_mboot;

    tables_initialize();
    hardware_initialize();

    terminal_initialize();

    if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
    {
        terminal_printf("This system must be booted by a multiboot-compliant bootloader.\n");
        asm volatile("hlt");
    }

    terminal_printf("Multiboot magic number: 0x%x\n", magic);
    terminal_printf("Multiboot info structure: 0x%x\n", mboot_ptr);

    asm volatile("sti");
}