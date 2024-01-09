#ifndef _DISPLAY_H
#define _DISPLAY_H

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#define VGA_COLOR_BLACK 0
#define VGA_COLOR_BLUE 1
#define VGA_COLOR_GREEN 2
#define VGA_COLOR_CYAN 3
#define VGA_COLOR_RED 4
#define VGA_COLOR_MAGENTA 5
#define VGA_COLOR_BROWN 6
#define VGA_COLOR_LIGHT_GREY 7
#define VGA_COLOR_DARK_GREY 8
#define VGA_COLOR_LIGHT_BLUE 9
#define VGA_COLOR_LIGHT_GREEN 10
#define VGA_COLOR_LIGHT_CYAN 11
#define VGA_COLOR_LIGHT_RED 12
#define VGA_COLOR_LIGHT_MAGENTA 13
#define VGA_COLOR_LIGHT_BROWN 14
#define VGA_COLOR_WHITE 15

#define VGA_ENTRY_COLOR(fg, bg) ((uint8_t)(fg | bg << 4))
#define VGA_ENTRY(uc, color) ((uint16_t)(uc | color << 8))

#define TERMCOLOR_NORMAL VGA_ENTRY_COLOR(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK)
#define TERMCOLOR_WARN VGA_ENTRY_COLOR(VGA_COLOR_YELLOW, VGA_COLOR_BLACK)
#define TERMCOLOR_ERROR VGA_ENTRY_COLOR(VGA_COLOR_WHITE, VGA_COLOR_RED)
#define TERMCOLOR_SUCCESS VGA_ENTRY_COLOR(VGA_COLOR_BLACK, VGA_COLOR_GREEN)

size_t strlen(const char* str);
void terminal_initialize(void);
void terminal_putentryat(char c, uint8_t color, size_t x, size_t y);
void terminal_putchar(char c);
void terminal_write(const char* data, size_t size);
void terminal_writestring(const char* data);
void terminal_movecursor(uint8_t x, uint8_t y);
void terminal_printf(const char *format, ...);

#endif