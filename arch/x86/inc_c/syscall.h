#ifndef _SYSCALL_H
#define _SYSCALL_H

#include "inc_c/hardware.h"

#define SYSCALL_READ 0x00
#define SYSCALL_WRITE 0x01
#define SYSCALL_OPEN 0x02
#define SYSCALL_CLOSE 0x03

void syscall_handler(regs_t *regs);

#endif