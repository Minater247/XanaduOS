.section .text

.extern timer_interrupt_handler
.globl irq0

# Not the same file, but the same definition as in tables_asm.s
irq0:
    cli
    pusha
    pushl %esp
    pushl %ebp
    call timer_interrupt_handler

.globl jump_to_program
jump_to_program:
    movl 8(%esp), %ebx
    movl 4(%esp), %eax

    movl %ebx, %ebp
    movl %eax, %esp

    popa # This is the important bit, we restore whatever was on the stack

    sti
    iret

.globl read_eip
read_eip:
    movl (%esp), %eax
    ret