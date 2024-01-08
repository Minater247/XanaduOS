ENTRY(_start)

SECTIONS
{

    . = 0xC0200000;

    .text ALIGN(4K) : AT(ADDR(.text)-0xC0000000)
    {
        *(.multiboot)
        *(.text)
    }

    .rodata ALIGN (4K) : AT(ADDR(.rodata)-0xC0000000)
    {
        *(.rodata)
    }

    .bss ALIGN(4K) : AT(ADDR(.bss)-0xC0000000)
    {
        *(.COMMON)
        *(.bss)
        *(.initial_stack)
    }

    .data ALIGN(4K) : AT(ADDR(.data)-0xC0000000)
    {
        *(.data)
    }

    KERNEL_END = .;
}