#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>

#include "inc_c/io.h"
#include "inc_c/serial.h"
#include "inc_c/string.h"

int serial_initialize()
{
    outb(SERIAL_COM1_BASE + 1, 0x00);
    outb(SERIAL_COM1_BASE + 3, 0x80);
    outb(SERIAL_COM1_BASE + 0, 0x03);
    outb(SERIAL_COM1_BASE + 1, 0x00);
    outb(SERIAL_COM1_BASE + 3, 0x03);
    outb(SERIAL_COM1_BASE + 2, 0xC7);
    outb(SERIAL_COM1_BASE + 4, 0x0B);
    outb(SERIAL_COM1_BASE + 4, 0x1E);
    outb(SERIAL_COM1_BASE + 0, 0xAE);

    if (inb(SERIAL_COM1_BASE + 0) != 0xAE)
    {
        return 1;
    }

    outb(SERIAL_COM1_BASE + 4, 0x0F);
    return 0;
}

void serial_putchar(char c)
{
    while ((inb(SERIAL_COM1_BASE + 5) & 0x20) == 0);
    outb(SERIAL_COM1_BASE, c);
}

void serial_write(char *buf, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++)
    {
        while ((inb(SERIAL_COM1_BASE + 5) & 0x20) == 0);
        outb(SERIAL_COM1_BASE, buf[i]);
    }
}

void serial_writestring(char *buf)
{
    uint32_t len = strlen(buf);
    for (uint32_t i = 0; i < len; i++)
    {
        while ((inb(SERIAL_COM1_BASE + 5) & 0x20) == 0);
        outb(SERIAL_COM1_BASE, buf[i]);
    }
}

void serial_printf(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    char buffer[32];
    for (size_t i = 0; i < strlen(format); i++)
    {
        if (format[i] == '%')
        {
            switch (format[++i])
            {
            case 'd':
                itoa(va_arg(args, int), buffer, 10);
                serial_writestring(buffer);
                break;
            case 's':
                serial_writestring(va_arg(args, char *));
                break;
            case 'c':
                serial_putchar(va_arg(args, int));
                break;
            case 'x':
                itoa(va_arg(args, uint32_t), buffer, 16);
                serial_writestring(buffer);
                break;
            case '%':
                serial_putchar('%');
                break;
            case 'l':
                switch (format[++i])
                {
                case 'd':
                    // run itoa on two 32-bit ints
                    uint32_t a = va_arg(args, uint32_t);
                    uint32_t b = va_arg(args, uint32_t);
                    itoa(a, buffer, 10);
                    serial_writestring(buffer);
                    itoa(b, buffer, 10);
                    serial_writestring(buffer);
                    break;
                case 'x':
                    // run itoa on two 32-bit ints
                    a = va_arg(args, uint32_t);
                    b = va_arg(args, uint32_t);
                    itoa(a, buffer, 16);
                    serial_writestring(buffer);
                    itoa(b, buffer, 16);
                    serial_writestring(buffer);
                    break;
                }
                break;
            default:
                serial_putchar(format[i]);
                break;
            }
        }
        else
        {
            serial_putchar(format[i]);
        }
    }
}