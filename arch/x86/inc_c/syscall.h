#ifndef _SYSCALL_H
#define _SYSCALL_H

#include "inc_c/hardware.h"

#define SYSCALL_READ 0
#define SYSCALL_WRITE 1
#define SYSCALL_OPEN 2
#define SYSCALL_CLOSE 3
#define SYSCALL_FSTAT 5
#define SYSCALL_EXIT 60
#define SYSCALL_GETDENT 78

void syscall_handler(regs_t *regs);

#endif