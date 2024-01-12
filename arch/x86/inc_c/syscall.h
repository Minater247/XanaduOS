#ifndef _SYSCALL_H
#define _SYSCALL_H

#include "inc_c/hardware.h"

#define SYSCALL_WRITE 0x01

void syscall_handler(regs_t *regs);

#endif