#include <stdint.h>
#include <stdbool.h>

#include "inc_c/hardware.h"
#include "inc_c/io.h"
#include "inc_c/display.h"
#include "../../kernel/include/unused.h"
#include "inc_c/devices.h"
#include "../../kernel/include/filesystem.h"

char keypress_buffer[256];
uint8_t keypress_buffer_size = 0;

bool shift = false;
bool caps = false;

char kbdcodes[128] =
    {
        0, 27, '1', '2', '3', '4', '5', '6', '7', '8',    /* 9 */
        '9', '0', '-', '=', '\b',                         /* Backspace */
        '\t',                                             /* Tab */
        'q', 'w', 'e', 'r',                               /* 19 */
        't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',     /* Enter key */
        0,                                                /* 29   - Control */
        'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', /* 39 */
        '\'', '`', 0,                                     /* Left shift */
        '\\', 'z', 'x', 'c', 'v', 'b', 'n',               /* 49 */
        'm', ',', '.', '/', 0,                            /* Right shift */
        '*',
        0,   /* Alt */
        ' ', /* Space bar */
        0,   /* Caps lock */
        0,   /* 59 - F1 key ... > */
        0, 0, 0, 0, 0, 0, 0, 0,
        0, /* < ... F10 */
        0, /* 69 - Num lock*/
        0, /* Scroll Lock */
        0, /* Home key */
        0, /* Up Arrow */
        0, /* Page Up */
        '-',
        0, /* Left Arrow */
        0,
        0, /* Right Arrow */
        '+',
        0, /* 79 - End key*/
        0, /* Down Arrow */
        0, /* Page Down */
        0, /* Insert Key */
        0, /* Delete Key */
        0, 0, 0,
        0, /* F11 Key */
        0, /* F12 Key */
        0, /* All other keys are undefined */
};
char kbdcodes_shifted[128] = {
    0, 27, '!', '@', '#', '$', '%', '^', '&', '*',    /* 9 */
    '(', ')', '_', '+', '\b',                         /* Backspace */
    '\t',                                             /* Tab */
    'Q', 'W', 'E', 'R',                               /* 19 */
    'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',     /* Enter key */
    0,                                                /* 29   - Control */
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', /* 39 */
    '"', '~', 0,                                      /* Left shift */
    '|', 'Z', 'X', 'C', 'V', 'B', 'N',                /* 49 */
    'M', '<', '>', '?', 0,                            /* Right shift */
    '*',
    0,   /* Alt */
    ' ', /* Space bar */
    0,   /* Caps lock */
    0,   /* 59 - F1 key ... > */
    0, 0, 0, 0, 0, 0, 0, 0,
    0, /* < ... F10 */
    0, /* 69 - Num lock*/
    0, /* Scroll Lock */
    0, /* Home key */
    0, /* Up Arrow */
    0, /* Page Up */
    '-',
    0, /* Left Arrow */
    0,
    0, /* Right Arrow */
    '+',
    0, /* 79 - End key*/
    0, /* Down Arrow */
    0, /* Page Down */
    0, /* Insert Key */
    0, /* Delete Key */
    0, 0, 0,
    0, /* F11 Key */
    0, /* F12 Key */
    0, /* All other keys are undefined */
};

// keyboard (hardware keyboard, not USB/PS2/etc)
void keyboard_interrupt_handler(regs_t *r)
{
    UNUSED(r);
    uint8_t scancode = inb(0x60);
    if (scancode & 0x80)
    {
        // key released
        scancode &= 0x7F;
        if (scancode == 0x2A || scancode == 0x36)
        {
            shift = false;
        }
    }
    else
    {
        // key pressed
        if (scancode == 0x2A || scancode == 0x36)
        {
            shift = true;
        }
        else if (scancode == 0x3A)
        {
            caps = !caps;
        }

        if (keypress_buffer_size < 255)
        {
            keypress_buffer[keypress_buffer_size] = scancode;
            keypress_buffer_size++;
        }
        else
        {
            // buffer full, shift everything down
            for (int i = 0; i < 255; i++)
            {
                keypress_buffer[i] = keypress_buffer[i + 1];
            }
            keypress_buffer[255] = scancode;
        }
    }
}

uint8_t keyboard_getcode()
{
    asm volatile("cli");
    if (keypress_buffer_size > 0)
    {
        keypress_buffer_size--;
        char scancode = keypress_buffer[0];
        // shift everything down
        for (int i = 0; i < 255; i++)
        {
            keypress_buffer[i] = keypress_buffer[i + 1];
        }
        asm volatile("sti");
        return scancode;
    }
    asm volatile("sti");
    return 0;
}

char keyboard_getchar()
{
    uint8_t scancode = keyboard_getcode();
    if (scancode == 0)
    {
        return 0;
    }
    if (shift)
    {
        return kbdcodes_shifted[scancode];
    }
    if (caps)
    {
        if (kbdcodes[scancode] >= 'a' && kbdcodes[scancode] <= 'z')
        {
            return kbdcodes_shifted[scancode];
        }
    }
    return kbdcodes[scancode];
}

// typedef struct device {
//     char name[64];
//     uint32_t flags;
//     int (*read)(void *ptr, uint32_t size);
//     int (*write)(void *ptr, uint32_t size);
//     int (*seek)(uint32_t offset, uint8_t whence);
//     int (*tell)();
//     //other functions generally return 0/NULL on success and -1/NULL on failure since devices are not files
//     struct device *next;
// } device_t;

int kbd_device_read(void *ptr, uint32_t size)
{
    uint32_t written = 0;
    char code;
    while ((code = keyboard_getchar()) != 0) {
        *(char *)(ptr + written) = code;
        written++;
        if (written >= size) {
            return written;
        }
    }

    return written;
}

device_t kbd_device = {
    .name = "kbd0",
    .flags = 0,
    .read = kbd_device_read,
    .write = device_rw_empty,
    .seek = device_seek_empty,
    .tell = device_tell_empty,
    .next = NULL,
};

void keyboard_install()
{
    irq_install_handler(1, keyboard_interrupt_handler);

    //init buffer/vars
    keypress_buffer_size = 0;
    shift = false;
    caps = false;

    register_device(&kbd_device);
}