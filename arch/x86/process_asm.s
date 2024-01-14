.global switch_stack_and_jump
switch_stack_and_jump:
    xchg %bx, %bx
    movl 8(%esp), %ebx # second argument, the function pointer
    movl 4(%esp), %eax # first argument, a location to build a stack
    movl %eax, %esp # set the stack pointer to the location
    movl %esp, %ebp # by setting the EBP to the ESP, we configure a new stack
    sti # enable interrupts
    jmp *%ebx # jump to the function pointer

.global switch_stack
switch_stack:
    movl 8(%esp), %ebx # second argument, the stack frame pointer
    movl 4(%esp), %eax # first argument, the stack pointer
    xchg %bx, %bx

    movl %ebx, %ebp # restore the stack frame
    movl %eax, %esp # restore the top of the stack

    popal # restore all registers

    sti # OK for interrupts at this point
    iret # perform a return from interrupt since the last task was interrupted