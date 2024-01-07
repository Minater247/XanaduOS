/* Declare constants for the multiboot header. */
.set ALIGN,    1<<0             /* align loaded modules on page boundaries */
.set MEMINFO,  1<<1             /* provide memory map */
.set FLAGS,    ALIGN | MEMINFO  /* this is the Multiboot 'flag' field */
.set MAGIC,    0x1BADB002       /* 'magic number' lets bootloader find the header */
.set CHECKSUM, -(MAGIC + FLAGS) /* checksum of above, to prove we are multiboot */

.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM


.section .bss
.align 16
stack_bottom:
.skip 16384 # 16 KiB
stack_top:

.section .multiboot_text

.section .text
.global _start

.include "arch/x86/inc_asm/paging.s"

.type _start, @function
_start:
	mov $stack_top-0xC0000000, %esp

	call paging_init

	mov $kernel_main, %eax
	jmp *%eax
	/* Should never return, given that we're jumping from the lower half */
