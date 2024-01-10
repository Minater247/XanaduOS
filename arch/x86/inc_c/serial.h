#ifndef _SERIAL_H
#define _SERIAL_H

#include <stdint.h>
#include <stdarg.h>

#define SERIAL_COM1_BASE 0x3f8

int serial_initialize();
void serial_printf(const char *format, ...);

#endif