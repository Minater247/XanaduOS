#include <stdint.h>
#include <stddef.h>

#include "inc_c/serial.h"
#include "inc_c/hardware.h"
#include "../../kernel/include/filesystem.h"
#include "inc_c/process.h"
#include "inc_c/syscall.h"

void syscall_handler(regs_t *regs) {
    serial_printf("Got syscall: 0x%x\n", regs->eax);

    switch (regs->eax) {
        case SYSCALL_WRITE:
            file_descriptor_t *fd = current_process->fds[regs->ebx];
            if (fd != NULL) {
                fd->flags |= FILE_WRITTEN_FLAG;
                int written = fwrite((char *)regs->ecx, 1, regs->edx, fd);
                regs->eax = written;
            } else {
                regs->eax = -1;
            }
            break;
    };

    return;
}