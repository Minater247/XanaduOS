#ifndef _IRQ_H
#define _IRQ_H

#include <kernel/asmfuncs.h>

void irq_install_handler(int irq, void (*handler)(struct regs *r));
void irq_uninstall_handler(int irq);
void irq_install();

#endif