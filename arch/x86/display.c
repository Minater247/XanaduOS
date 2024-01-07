#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "inc_c/display.h"
#include "inc_c/io.h"

size_t strlen(const char *str)
{
    size_t len = 0;
    while (str[len])
        len++;
    return len;
}

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

void terminal_setcolor(uint8_t color)
{
    terminal_color = color;
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
        //scroll the buffer if we're at the bottom
        if (++terminal_row == VGA_HEIGHT)
        {
            terminal_row = VGA_HEIGHT - 2;
            for (size_t y = 0; y < VGA_HEIGHT - 1; y++)
            {
                for (size_t x = 0; x < VGA_WIDTH; x++)
                {
                    const size_t index = y * VGA_WIDTH + x;
                    terminal_buffer[index] = terminal_buffer[index + VGA_WIDTH];
                }
            }
            //clear the last line
            for (size_t x = 0; x < VGA_WIDTH; x++)
            {
                const size_t index = (VGA_HEIGHT - 1) * VGA_WIDTH + x;
                terminal_buffer[index] = VGA_ENTRY(' ', terminal_color);
            }
        }
        return;
    }
    terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
    if (++terminal_column == VGA_WIDTH)
    {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT)
            terminal_row = 0;
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
    terminal_movecursor(terminal_column, terminal_row);
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

void terminal_write_hex(uint32_t num) {
    char hex[8];
    int i = 0;

    for (i = 0; i < 8; i++) {
        hex[i] = num & 0xF;
        num >>= 4;
    }

    for (i = 7; i >= 0; i--) {
        if (hex[i] < 10) {
            terminal_putchar(hex[i] + '0');
        } else {
            terminal_putchar(hex[i] - 10 + 'A');
        }
    }
}