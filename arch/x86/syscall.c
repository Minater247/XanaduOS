#include <stdint.h>
#include <stddef.h>

#include "inc_c/serial.h"
#include "inc_c/hardware.h"
#include "../../kernel/include/filesystem.h"
#include "inc_c/process.h"
#include "inc_c/syscall.h"
#include "inc_c/process.h"
#include "inc_c/string.h"
#include "../../kernel/include/errors.h"



typedef void (*syscall_handler_t)(regs_t *regs);

//table of syscall handlers
syscall_handler_t syscall_handlers[256];

void syscall_handler(regs_t *regs) {
    serial_printf("Syscall: 0x%x\n", regs->eax);

    syscall_handler_t handler = syscall_handlers[regs->eax];
    if (regs->eax < 256 && handler != NULL) {
        handler(regs);
    } else {
        serial_printf("Invalid syscall: 0x%x\n", regs->eax);
        regs->eax = -1;
    }   

    return;
}

void syscall_read(regs_t *regs) {
    file_descriptor_t *fd = current_process->fds[regs->ebx];
    if (fd != NULL) {
        int read = fread((char *)regs->ecx, 1, regs->edx, fd);
        regs->eax = read;
    } else {
        regs->eax = -1;
    }
}

void syscall_write(regs_t *regs) {
    file_descriptor_t *fd = current_process->fds[regs->ebx];
    if (fd != NULL) {
        fd->flags |= FILE_WRITTEN_FLAG;
        int written = fwrite((char *)regs->ecx, 1, regs->edx, fd);
        regs->eax = written;
    } else {
        regs->eax = -1;
    }
}

void syscall_open(regs_t *regs) {
    file_descriptor_t *fd = fopen((char *)regs->ebx, (char*)regs->ecx);
    if (fd->flags & FILE_ISDIR_FLAG) {
        fclose(fd);
        fd = (file_descriptor_t *)fopendir((char *)regs->ebx);
        regs->eax = fd->id;
    } else if (fd->flags & FILE_NOTFOUND_FLAG) {
        fclose(fd);
        regs->eax = -1;
    } else {
        regs->eax = fd->id;
    }
}

void syscall_close(regs_t *regs) {
    file_descriptor_t *fd = current_process->fds[regs->ebx];
    if (fd != NULL) {
        fclose(fd);
        regs->eax = 0;
    } else {
        regs->eax = -1;
    }
}

void syscall_fstat(regs_t *regs) {
    file_descriptor_t *fd = current_process->fds[regs->ebx];
    if (fd != NULL) {
        regs->eax = fd->fs->stat(fd->fs_data, (stat_t *)regs->ecx);
    } else {
        regs->eax = -1;
    }
}

void syscall_exit(regs_t *regs) {
    current_process->entry_or_return = regs->ebx;
    free_process(current_process);
}

void syscall_getdent(regs_t *regs) {
    file_descriptor_t *fd = current_process->fds[regs->ebx];
    if (fd != NULL) {
        uint32_t ret = (uint32_t)fd->fs->getdent((dirent_t *)regs->ecx, regs->edx, fd->fs_data);
        regs->eax = ret;
    } else {
        regs->eax = -1;
    }
}

void syscall_initialize() {
    memset(syscall_handlers, 0, sizeof(syscall_handlers));
    syscall_handlers[SYSCALL_READ] = syscall_read;
    syscall_handlers[SYSCALL_WRITE] = syscall_write;
    syscall_handlers[SYSCALL_OPEN] = syscall_open;
    syscall_handlers[SYSCALL_CLOSE] = syscall_close;
    syscall_handlers[SYSCALL_FSTAT] = syscall_fstat;
    syscall_handlers[SYSCALL_EXIT] = syscall_exit;
    syscall_handlers[SYSCALL_GETDENT] = syscall_getdent;
}