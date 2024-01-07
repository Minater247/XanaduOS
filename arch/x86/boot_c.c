#include <stdint.h>

#include "inc_c/boot.h"
#include "inc_c/display.h"
#include "inc_c/tables.h"
#include "inc_c/hardware.h"

// This is the first function called by the kernel.
// This function should:
// - Initialize all processor systems (GDT, IDT, PIC, etc.)
// - Initialize all kernel systems (terminal, memory, etc.)
// - Initialize all drivers (keyboard, mouse, etc.)
// - Initialize all kernel modules (filesystem, etc.)
// - Initialize multitasking components (processes, threads, etc.) if available
void boot_initialize() {
    tables_initialize();
    hardware_initialize();

    terminal_initialize();

    asm volatile("sti");
}