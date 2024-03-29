.equ VIRTUAL_ADDRESS, 0xC0000000
.equ PAGE_SIZE, 0x1000

.section .text
.global paging_init
paging_init:
    /* Set up the page directory */
    /* x86 base support is only for 4kb pages, so we will use those to map the first 8MB. */
    /* If the kernel grows beyond 8MB, we will need to add more page tables. */
    mov $page_directory-VIRTUAL_ADDRESS, %ecx
    mov %ecx, %cr3

    /* Set up the first 8MB of address space */
    mov $boot_pt1-VIRTUAL_ADDRESS, %ecx
    or $0x3, %ecx
    mov %ecx, page_directory-VIRTUAL_ADDRESS+0x0
    mov %ecx, page_directory-VIRTUAL_ADDRESS+0xC00
    mov $boot_pt2-VIRTUAL_ADDRESS, %ecx
    or $0x3, %ecx
    mov %ecx, page_directory-VIRTUAL_ADDRESS+0x4
    mov %ecx, page_directory-VIRTUAL_ADDRESS+0xC04

    /* Enable paging */
    mov %cr0, %ecx
    or $0x80000000, %ecx
    mov %ecx, %cr0

    /* bump stack up by 3GB */
    add $0xC0000000, %esp

    ret


.section .data
.align 4096
.global page_directory
page_directory:
    .rept 1024
    .long 0x00000002
    .endr
.include "arch/x86/inc_asm/pagetables.s"
