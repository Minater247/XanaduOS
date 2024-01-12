#include <stdint.h>

#include "inc_c/display.h"
#include "inc_c/hardware.h"

#include "inc_c/syscall.h"

void syscall_handler(regs_t *regs) {
    terminal_printf("Syscall! %d\n", regs->eax);

    switch (regs->eax) {
        case SYSCALL_WRITE:
            terminal_printf("Syscall write! 0x%x -> %d, %d\n", regs->esi, regs->edi, regs->edx);
            terminal_write((char *)regs->esi, regs->edx);
            break;
    };

    return;
}