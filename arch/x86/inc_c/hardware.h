#ifndef _HARDWARE_H
#define _HARDWARE_H

#include <stdint.h>

#include "inc_c/hardware.h"

typedef struct {
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
} __attribute__((packed)) regs_t;

void hardware_initialize();
void irq_install_handler(int irq, void (*handler)(regs_t *r));
void irq_uninstall_handler(int irq);

#endif