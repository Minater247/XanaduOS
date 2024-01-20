#include <stdint.h>
#include <stddef.h>

#include "inc_c/serial.h"
#include "inc_c/hardware.h"
#include "../../kernel/include/filesystem.h"
#include "inc_c/process.h"
#include "inc_c/syscall.h"
#include "inc_c/process.h"
#include "../../kernel/include/errors.h"

void syscall_handler(regs_t *regs) {
    //serial_printf("Syscall: 0x%x\n", regs->eax);

    file_descriptor_t *fd;

    switch (regs->eax) {
        case SYSCALL_READ:
            fd = current_process->fds[regs->ebx];
            if (fd != NULL) {
                int read = fread((char *)regs->ecx, 1, regs->edx, fd);
                regs->eax = read;
            } else {
                regs->eax = -1;
            }
            break;
        case SYSCALL_WRITE:
            fd = current_process->fds[regs->ebx];
            if (fd != NULL) {
                fd->flags |= FILE_WRITTEN_FLAG;
                int written = fwrite((char *)regs->ecx, 1, regs->edx, fd);
                regs->eax = written;
            } else {
                regs->eax = -1;
            }
            break;
        case SYSCALL_OPEN:
            fd = fopen((char *)regs->ebx, (char*)regs->ecx);
            if (fd->flags & FILE_ISDIR_FLAG) {
                fclose(fd);
                fd = (file_descriptor_t *)fopendir((char *)regs->ebx);
                regs->eax = fd->id;
            } else if (fd->flags & FILE_NOTFOUND_FLAG) {
                regs->eax = -1;
            } else {
                regs->eax = fd->id;
            }
            break;
        case SYSCALL_CLOSE:
            fd = current_process->fds[regs->ebx];
            if (fd != NULL) {
                fclose(fd);
                regs->eax = 0;
            } else {
                regs->eax = -1;
            }
            break;
        case SYSCALL_FSTAT:
            fd = current_process->fds[regs->ebx];
            if (fd != NULL) {
                regs->eax = fd->fs->stat(fd->fs_data, (stat_t *)regs->ecx);
            } else {
                regs->eax = -1;
            }
            break;
        case SYSCALL_EXIT:
            current_process->entry_or_return = regs->ebx;
            free_process(current_process);
            break;
        case SYSCALL_GETDENT:
            fd = current_process->fds[regs->ebx];
            if (fd != NULL) {
                uint32_t ret = (uint32_t)fd->fs->getdent((dirent_t *)regs->ecx, regs->edx, fd->fs_data);
                regs->eax = ret;
            } else {
                regs->eax = -1;
            }
            break;
        default:
            serial_printf("Unknown syscall: 0x%x\n", regs->eax);
            break;
    };

    //serial_printf("Returning from syscall.\n");

    return;
}