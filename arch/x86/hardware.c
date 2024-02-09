#include <stdint.h>

#include "inc_c/hardware.h"
#include "drivers/keyboard.h"
#include "inc_c/syscall.h"
#include "drivers/PIT.h"

void hardware_initialize()
{
    syscall_initialize();
    timer_install();
    keyboard_install();
}