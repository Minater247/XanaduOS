#include <stdint.h>

#include "../../kernel/include/unused.h"
#include "inc_c/io.h"
#include "inc_c/display.h"
#include "inc_c/tables.h"
#include "inc_c/hardware.h"

//PIT
void timer_phase(int hz)
{
    int divisor = 1193180 / hz;
    outb(0x43, 0x36);
    outb(0x40, divisor & 0xFF);
    outb(0x40, divisor >> 8);
}

void timer_interrupt_handler(regs_t *r)
{
    UNUSED(r);
    static uint32_t tick = 0;
    tick++;
    if (tick % 18 == 0)
    {
        terminal_writestring("Tick\n");
    }
}

void timer_install()
{
    irq_install_handler(0, timer_interrupt_handler);
    timer_phase(100);
}