#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

#include "inc_c/display.h"
#include "inc_c/io.h"
#include "inc_c/string.h"

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;

size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;
uint16_t *terminal_buffer;

void terminal_initialize(void)
{
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = TERMCOLOR_NORMAL;
    terminal_buffer = (uint16_t *)0xC00B8000;
    for (size_t y = 0; y < VGA_HEIGHT; y++)
    {
        for (size_t x = 0; x < VGA_WIDTH; x++)
        {
            const size_t index = y * VGA_WIDTH + x;
            terminal_buffer[index] = VGA_ENTRY(' ', terminal_color);
        }
    }
}

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y)
{
    const size_t index = y * VGA_WIDTH + x;
    terminal_buffer[index] = VGA_ENTRY(c, color);
}

void terminal_putchar(char c)
{
    if (c == '\n')
    {
        terminal_column = 0;
        // scroll the buffer if we're at the bottom
        if (++terminal_row == VGA_HEIGHT)
        {
            terminal_row = VGA_HEIGHT - 1;
            for (size_t y = 0; y < VGA_HEIGHT - 1; y++)
            {
                for (size_t x = 0; x < VGA_WIDTH; x++)
                {
                    const size_t index = y * VGA_WIDTH + x;
                    terminal_buffer[index] = terminal_buffer[index + VGA_WIDTH];
                }
            }
            // clear the last line
            for (size_t x = 0; x < VGA_WIDTH; x++)
            {
                const size_t index = (VGA_HEIGHT - 1) * VGA_WIDTH + x;
                terminal_buffer[index] = VGA_ENTRY(' ', terminal_color);
            }
        }
        return;
    }
    else if (c == '\b')
    {
        if (terminal_column > 0)
        {
            terminal_column--;
        }
        else
        {
            if (terminal_row > 0)
            {
                terminal_row--;
                terminal_column = VGA_WIDTH - 1;
            }
        }
        terminal_putentryat(' ', terminal_color, terminal_column, terminal_row);
        return;
    } else if (c == '\r')
    {
        terminal_column = 0;
        return;
    }
    terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
    if (++terminal_column == VGA_WIDTH)
    {
        terminal_write("\n", 1);
    }
}

void terminal_write(const char *data, size_t size)
{
    for (size_t i = 0; i < size; i++)
        terminal_putchar(data[i]);
}

void terminal_writestring(const char *data)
{
    terminal_write(data, strlen(data));
}

void terminal_movecursor(uint8_t x, uint8_t y)
{
    uint16_t pos = y * VGA_WIDTH + x;

    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void terminal_setpos(uint8_t x, uint8_t y)
{
    terminal_column = x;
    terminal_row = y;
    terminal_movecursor(x, y);
}

void terminal_printf(const char *format, ...)
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
                terminal_writestring(buffer);
                break;
            case 's':
                terminal_writestring(va_arg(args, char *));
                break;
            case 'c':
                terminal_putchar(va_arg(args, int));
                break;
            case 'x':
                itoa(va_arg(args, uint32_t), buffer, 16);
                terminal_writestring(buffer);
                break;
            case '%':
                terminal_putchar('%');
                break;
            case 'l':
                switch (format[++i])
                {
                case 'd':
                    // run itoa on two 32-bit ints
                    uint32_t a = va_arg(args, uint32_t);
                    uint32_t b = va_arg(args, uint32_t);
                    itoa(a, buffer, 10);
                    terminal_writestring(buffer);
                    itoa(b, buffer, 10);
                    terminal_writestring(buffer);
                    break;
                case 'x':
                    // run itoa on two 32-bit ints
                    a = va_arg(args, uint32_t);
                    b = va_arg(args, uint32_t);
                    itoa(a, buffer, 16);
                    terminal_writestring(buffer);
                    itoa(b, buffer, 16);
                    terminal_writestring(buffer);
                    break;
                }
                break;
            default:
                terminal_putchar(format[i]);
                break;
            }
        }
        else if (format[i] == '\033')
        {
            //read forward until \0 or a non-numeric/; character
            uint32_t end_i = i + 1;
            if (format[end_i] == '[')
            {
                end_i++;
            } else {
                // not a valid escape sequence
                terminal_putchar(format[i]);
                continue;
            }
            while (((format[end_i] >= '0' && format[end_i] <= '9') || format[end_i] == ';') && format[end_i] != '\0')
            {
                end_i++;
            }
            //make sure we aren't at the end of the string
            if (format[end_i] == '\0') {
                break;
            }

            //depending on the character, process the sequence
            if (format[end_i] == 'm')
            {
                //color sequence, process each semi-colon separated number
                uint32_t start_i = i + 2;
                while (start_i < end_i)
                {
                    //read the number
                    uint32_t num = 0;
                    while (format[start_i] >= '0' && format[start_i] <= '9')
                    {
                        num *= 10;
                        num += format[start_i] - '0';
                        start_i++;
                    }
                    //process the number (this is annoyingly long but it works for now)
                    switch (num)
                    {
                    case 0:
                        //reset
                        terminal_color = TERMCOLOR_NORMAL;
                        break;
                    case 1:
                        //bold
                        terminal_color |= 0x08;
                        break;
                    case 4:
                        //underline
                        terminal_color |= 0x80;
                        break;
                    case 30:
                        //black fg
                        terminal_color &= 0xF0;
                        break;
                    case 31:
                        //red fg
                        terminal_color &= 0xF0;
                        terminal_color |= 0x04;
                        break;
                    case 32:
                        //green fg
                        terminal_color &= 0xF0;
                        terminal_color |= 0x02;
                        break;
                    case 33:
                        //yellow fg
                        terminal_color &= 0xF0;
                        terminal_color |= 0x06;
                        break;
                    case 34:
                        //blue fg
                        terminal_color &= 0xF0;
                        terminal_color |= 0x01;
                        break;
                    case 35:
                        //magenta fg
                        terminal_color &= 0xF0;
                        terminal_color |= 0x05;
                        break;
                    case 36:
                        //cyan fg
                        terminal_color &= 0xF0;
                        terminal_color |= 0x03;
                        break;
                    case 37:
                        //white fg
                        terminal_color &= 0xF0;
                        terminal_color |= 0x07;
                        break;
                    case 40:
                        //black bg
                        terminal_color &= 0x0F;
                        break;
                    case 41:
                        //red bg
                        terminal_color &= 0x0F;
                        terminal_color |= 0x40;
                        break;
                    case 42:
                        //green bg
                        terminal_color &= 0x0F;
                        terminal_color |= 0x20;
                        break;
                    case 43:
                        //yellow bg
                        terminal_color &= 0x0F;
                        terminal_color |= 0x60;
                        break;
                    case 44:
                        //blue bg
                        terminal_color &= 0x0F;
                        terminal_color |= 0x10;
                        break;
                    case 45:
                        //magenta bg
                        terminal_color &= 0x0F;
                        terminal_color |= 0x50;
                        break;
                    case 46:
                        //cyan bg
                        terminal_color &= 0x0F;
                        terminal_color |= 0x30;
                        break;
                    case 47:
                        //white bg
                        terminal_color &= 0x0F;
                        terminal_color |= 0x70;
                        break;
                    case 90:
                        //light black fg
                        terminal_color &= 0xF0;
                        terminal_color |= 0x08;
                        break;
                    case 91:
                        //light red fg
                        terminal_color &= 0xF0;
                        terminal_color |= 0x0C;
                        break;
                    case 92:
                        //light green fg
                        terminal_color &= 0xF0;
                        terminal_color |= 0x0A;
                        break;
                    case 93:
                        //light yellow fg
                        terminal_color &= 0xF0;
                        terminal_color |= 0x0E;
                        break;
                    case 94:
                        //light blue fg
                        terminal_color &= 0xF0;
                        terminal_color |= 0x09;
                        break;
                    case 95:
                        //light magenta fg
                        terminal_color &= 0xF0;
                        terminal_color |= 0x0D;
                        break;
                    case 96:
                        //light cyan fg
                        terminal_color &= 0xF0;
                        terminal_color |= 0x0B;
                        break;
                    case 97:
                        //light white fg
                        terminal_color &= 0xF0;
                        terminal_color |= 0x0F;
                        break;
                    case 100:
                        //light black bg
                        terminal_color &= 0x0F;
                        terminal_color |= 0x80;
                        break;
                    case 101:
                        //light red bg
                        terminal_color &= 0x0F;
                        terminal_color |= 0xC0;
                        break;
                    case 102:
                        //light green bg
                        terminal_color &= 0x0F;
                        terminal_color |= 0xA0;
                        break;
                    case 103:
                        //light yellow bg
                        terminal_color &= 0x0F;
                        terminal_color |= 0xE0;
                        break;
                    case 104:
                        //light blue bg
                        terminal_color &= 0x0F;
                        terminal_color |= 0x90;
                        break;
                    case 105:
                        //light magenta bg
                        terminal_color &= 0x0F;
                        terminal_color |= 0xD0;
                        break;
                    case 106:
                        //light cyan bg
                        terminal_color &= 0x0F;
                        terminal_color |= 0xB0;
                        break;
                    case 107:
                        //light white bg
                        terminal_color &= 0x0F;
                        terminal_color |= 0xF0;
                        break;
                    }
                    start_i++;
                }
                //move the index to the end of the sequence
                i = end_i;
            }
        }
        else
        {
            terminal_putchar(format[i]);
        }
    }
    terminal_movecursor(terminal_column, terminal_row);
}