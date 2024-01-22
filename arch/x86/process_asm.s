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

.global init_program
init_program:
    push %ebp
    mov %esp, %ebp

    # The parameters are on the stack, above the saved EBP
    # argc is at 8(%ebp), argv is at 12(%ebp), envp is at 16(%ebp), and entry_point is at 20(%ebp)

    # Push the parameters for the entry_point function onto the stack
    mov 16(%ebp), %eax
    push %eax
    mov 12(%ebp), %eax
    push %eax
    mov 8(%ebp), %eax
    push %eax

    # Call the entry_point function
    call *20(%ebp)

    # Clean up the stack
    add $8, %esp

    pop %ebp
    ret