#include <stdint.h>

#include "inc_c/boot.h"
#include "inc_c/display.h"
#include "inc_c/tables.h"
#include "inc_c/hardware.h"
#include "inc_c/multiboot.h"
#include "inc_c/memory.h"
#include "inc_c/string.h"
#include "../../kernel/include/errors.h"

extern uint32_t given_magic;
extern uint32_t given_mboot;
extern uint32_t page_directory[1024];

// This is the first function called by the kernel.
// This function should:
// - Initialize all processor systems (GDT, IDT, PIC, etc.)
// - Initialize all kernel systems (terminal, memory, etc.)
// - Initialize all drivers (keyboard, mouse, etc.)
// - Initialize all kernel modules (filesystem, etc.)
// - Initialize multitasking components (processes, threads, etc.) if available
void boot_initialize() {
    uint32_t magic = given_magic;
    multiboot_info_t *mboot_info = (multiboot_info_t *)(given_mboot + 0xC0000000);

    tables_initialize();
    hardware_initialize();

    terminal_initialize();

    if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
    {
        terminal_printf("This system must be booted by a multiboot-compliant bootloader.\n");
        asm volatile("hlt");
    }

    //loved the ToaruOS multiboot info dump, so I'm doing it too
    terminal_printf("\033[94mMultiboot information:\n");
    char buf[9]; //for left-filling hex numbers to 8 digits

    terminal_printf("\033[0mFlags : 0x");
    memset(buf, 0, 8);
    itoa(mboot_info->flags, buf, 16);
    while (buf[7] == 0)
    {
        for (int i = 7; i > 0; i--)
        {
            buf[i] = buf[i - 1];
        }
        buf[0] = '0';
    }
    terminal_printf("%s ", buf);

    terminal_printf("Mem Lo: 0x");
    memset(buf, 0, 8);
    itoa(mboot_info->mem_lower, buf, 16);
    while (buf[7] == 0)
    {
        for (int i = 7; i > 0; i--)
        {
            buf[i] = buf[i - 1];
        }
        buf[0] = '0';
    }
    terminal_printf("%s ", buf);

    terminal_printf("Mem Hi: 0x");
    memset(buf, 0, 8);
    itoa(mboot_info->mem_upper, buf, 16);
    while (buf[7] == 0)
    {
        for (int i = 7; i > 0; i--)
        {
            buf[i] = buf[i - 1];
        }
        buf[0] = '0';
    }
    terminal_printf("%s ", buf);

    terminal_printf("Boot D: 0x");
    memset(buf, 0, 8);
    itoa(mboot_info->boot_device, buf, 16);
    while (buf[7] == 0)
    {
        for (int i = 7; i > 0; i--)
        {
            buf[i] = buf[i - 1];
        }
        buf[0] = '0';
    }
    terminal_printf("%s\n", buf);

    terminal_printf("Cmdlin: 0x");
    memset(buf, 0, 8);
    itoa(mboot_info->cmdline, buf, 16);
    while (buf[7] == 0)
    {
        for (int i = 7; i > 0; i--)
        {
            buf[i] = buf[i - 1];
        }
        buf[0] = '0';
    }
    terminal_printf("%s ", buf);

    terminal_printf("Mods  : 0x");
    memset(buf, 0, 8);
    itoa(mboot_info->mods_count, buf, 16);
    while (buf[7] == 0)
    {
        for (int i = 7; i > 0; i--)
        {
            buf[i] = buf[i - 1];
        }
        buf[0] = '0';
    }
    terminal_printf("%s ", buf);

    terminal_printf("ModAdd: 0x");
    memset(buf, 0, 8);
    itoa(mboot_info->mods_addr, buf, 16);
    while (buf[7] == 0)
    {
        for (int i = 7; i > 0; i--)
        {
            buf[i] = buf[i - 1];
        }
        buf[0] = '0';
    }
    terminal_printf("%s ", buf);

    terminal_printf("Syms  : 0x");
    memset(buf, 0, 8);
    itoa(mboot_info->u.elf_sec.num, buf, 16);
    while (buf[7] == 0)
    {
        for (int i = 7; i > 0; i--)
        {
            buf[i] = buf[i - 1];
        }
        buf[0] = '0';
    }
    terminal_printf("%s\n", buf);

    terminal_printf("SymsAd: 0x");
    memset(buf, 0, 8);
    itoa(mboot_info->u.elf_sec.addr, buf, 16);
    while (buf[7] == 0)
    {
        for (int i = 7; i > 0; i--)
        {
            buf[i] = buf[i - 1];
        }
        buf[0] = '0';
    }
    terminal_printf("%s ", buf);

    terminal_printf("SymsNb: 0x");
    memset(buf, 0, 8);
    itoa(mboot_info->u.elf_sec.size, buf, 16);
    while (buf[7] == 0)
    {
        for (int i = 7; i > 0; i--)
        {
            buf[i] = buf[i - 1];
        }
        buf[0] = '0';
    }
    terminal_printf("%s ", buf);

    terminal_printf("SymsSz: 0x");
    memset(buf, 0, 8);
    itoa(mboot_info->u.elf_sec.shndx, buf, 16);
    while (buf[7] == 0)
    {
        for (int i = 7; i > 0; i--)
        {
            buf[i] = buf[i - 1];
        }
        buf[0] = '0';
    }
    terminal_printf("%s ", buf);

    terminal_printf("Mmap  : 0x");
    memset(buf, 0, 8);
    itoa(mboot_info->mmap_length, buf, 16);
    while (buf[7] == 0) {
        for (int i = 7; i > 0; i--) {
            buf[i] = buf[i - 1];
        }
        buf[0] = '0';
    }
    terminal_printf("%s\n", buf);

    terminal_printf("MmapAd: 0x");
    memset(buf, 0, 8);
    itoa(mboot_info->mmap_addr, buf, 16);
    while (buf[7] == 0) {
        for (int i = 7; i > 0; i--) {
            buf[i] = buf[i - 1];
        }
        buf[0] = '0';
    }
    terminal_printf("%s ", buf);

    terminal_printf("Drives: 0x");
    memset(buf, 0, 8);
    itoa(mboot_info->drives_length, buf, 16);
    while (buf[7] == 0) {
        for (int i = 7; i > 0; i--) {
            buf[i] = buf[i - 1];
        }
        buf[0] = '0';
    }
    terminal_printf("%s ", buf);

    terminal_printf("DrvAdr: 0x");
    memset(buf, 0, 8);
    itoa(mboot_info->drives_addr, buf, 16);
    while (buf[7] == 0) {
        for (int i = 7; i > 0; i--) {
            buf[i] = buf[i - 1];
        }
        buf[0] = '0';
    }
    terminal_printf("%s ", buf);

    terminal_printf("Config: 0x");
    memset(buf, 0, 8);
    itoa(mboot_info->config_table, buf, 16);
    while (buf[7] == 0) {
        for (int i = 7; i > 0; i--) {
            buf[i] = buf[i - 1];
        }
        buf[0] = '0';
    }
    terminal_printf("%s\n", buf);

    terminal_printf("Loader: 0x");
    memset(buf, 0, 8);
    itoa(mboot_info->boot_loader_name, buf, 16);
    while (buf[7] == 0) {
        for (int i = 7; i > 0; i--) {
            buf[i] = buf[i - 1];
        }
        buf[0] = '0';
    }
    terminal_printf("%s ", buf);

    terminal_printf("APM   : 0x");
    memset(buf, 0, 8);
    itoa(mboot_info->apm_table, buf, 16);
    while (buf[7] == 0) {
        for (int i = 7; i > 0; i--) {
            buf[i] = buf[i - 1];
        }
        buf[0] = '0';
    }
    terminal_printf("%s ", buf);

    terminal_printf("VBE Co: 0x");
    memset(buf, 0, 8);
    itoa(mboot_info->vbe_control_info, buf, 16);
    while (buf[7] == 0) {
        for (int i = 7; i > 0; i--) {
            buf[i] = buf[i - 1];
        }
        buf[0] = '0';
    }
    terminal_printf("%s ", buf);

    terminal_printf("VBE Mo: 0x");
    memset(buf, 0, 8);
    itoa(mboot_info->vbe_mode_info, buf, 16);
    while (buf[7] == 0) {
        for (int i = 7; i > 0; i--) {
            buf[i] = buf[i - 1];
        }
        buf[0] = '0';
    }
    terminal_printf("%s\n", buf);

    terminal_printf("VBE In: 0x");
    memset(buf, 0, 8);
    itoa(mboot_info->vbe_mode, buf, 16);
    while (buf[7] == 0) {
        for (int i = 7; i > 0; i--) {
            buf[i] = buf[i - 1];
        }
        buf[0] = '0';
    }
    terminal_printf("%s ", buf);

    terminal_printf("VBE Se: 0x");
    memset(buf, 0, 8);
    itoa(mboot_info->vbe_interface_seg, buf, 16);
    while (buf[7] == 0) {
        for (int i = 7; i > 0; i--) {
            buf[i] = buf[i - 1];
        }
        buf[0] = '0';
    }
    terminal_printf("%s ", buf);

    terminal_printf("VBE Of: 0x");
    memset(buf, 0, 8);
    itoa(mboot_info->vbe_interface_off, buf, 16);
    while (buf[7] == 0) {
        for (int i = 7; i > 0; i--) {
            buf[i] = buf[i - 1]; 
        }
        buf[0] = '0';
    }
    terminal_printf("%s ", buf);

    terminal_printf("VBE Le: 0x");
    memset(buf, 0, 8);
    itoa(mboot_info->vbe_interface_len, buf, 16);
    while (buf[7] == 0) { 
        for (int i = 7; i > 0; i--) { 
            buf[i] = buf[i - 1];
        }
        buf[0] = '0';
    }
    terminal_printf("%s\n", buf);

    terminal_printf("\033[97mEnd of multiboot information.\n");

    terminal_printf("Booted from %s.\n", mboot_info->boot_loader_name);
    terminal_printf("%dKB lower memory\n", mboot_info->mem_lower);
    terminal_printf("%dKB upper memory (%dMB)\n", mboot_info->mem_upper, mboot_info->mem_upper / 1024);

    terminal_printf("\033[0mTesting colors...\n");
    terminal_printf("\033[40m \033[41m \033[42m \033[43m \033[44m \033[45m \033[46m \033[47m \033[100m \033[101m \033[102m \033[103m \033[104m \033[105m \033[106m \033[107m \n");

    terminal_printf("\033[0mHeap time!\n");

    memory_initialize(mboot_info);

    asm volatile("sti");
}