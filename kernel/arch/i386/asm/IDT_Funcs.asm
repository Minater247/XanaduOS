[BITS 32]

; A list of interrupts can be found at http://www.osdever.net/bkerndev/Docs/isrs.htm
; Thank you Brandon!

; Loads the IDT defined in '_idtp' into the processor.
; This is declared in C as 'extern void idt_load();'
GLOBAL idt_load
EXTERN idtp

SECTION .text
idt_load:
    lidt [idtp]
    ret

; Service Routines (ISRs) right here!
global isr0
global isr1
global isr2
global isr3
global isr4
global isr5
global isr6
global isr7
global isr8
global isr9
global isr10
global isr11
global isr12
global isr13
global isr14
global isr15
global isr16
global isr17
global isr18
global isr19
global isr20
global isr21
global isr22
global isr23
global isr24
global isr25
global isr26
global isr27
global isr28
global isr29
global isr30
global isr31

;  0: Divide By Zero Exception
isr0:
    cli
    push byte 0    ; A normal ISR stub that pops a dummy error code to keep a
                   ; uniform stack frame
    push byte 0
    jmp isr_common_stub

;  1: Debug Exception
isr1:
    cli
    push byte 0
    push byte 1
    jmp isr_common_stub

;  2: Non Maskable Interrupt
isr2:
    cli
    push byte 0
    push byte 2
    jmp isr_common_stub

;  3: Breakpoint Exception
isr3:
    cli
    push byte 0
    push byte 3
    jmp isr_common_stub

;  4: Overflow Exception
isr4:
    cli
    push byte 0
    push byte 4
    jmp isr_common_stub

;  5: Bounds Check Exception
isr5:
    cli
    push byte 0
    push byte 5
    jmp isr_common_stub

;  6: Invalid Opcode Exception
isr6:
    cli
    push byte 0
    push byte 6
    jmp isr_common_stub

;  7: Coprocessor Not Available Exception
isr7:
    cli
    push byte 0
    push byte 7
    jmp isr_common_stub

;  8: Double Fault Exception (With Error Code!)
isr8:
    cli
    push byte 8        ; Note that we DON'T push a value on the stack in this one!
                   ; It pushes one already! Use this type of stub for exceptions
                   ; that pop error codes!
    jmp isr_common_stub

; 9: Coprocessor Segment Overrun Exception
isr9:
    cli
    push byte 0
    push byte 9
    jmp isr_common_stub

;  10: Invalid Task State Segment Exception (With Error Code!)
isr10:
    cli
    push byte 0
    jmp isr_common_stub

;  11: Segment Not Present Exception (With Error Code!)
isr11:
    cli
    push byte 0
    jmp isr_common_stub

;  12: Stack Fault Exception (With Error Code!)
isr12:
    cli
    push byte 0
    jmp isr_common_stub

;  13: General Protection Fault Exception (With Error Code!)
isr13:
    cli
    push byte 0
    jmp isr_common_stub

;  14: Page Fault Exception (With Error Code!)
isr14:
    cli
    push byte 0
    jmp isr_common_stub

;  15: Unknown Interrupt Exception
isr15:
    cli
    push byte 0
    push byte 15
    jmp isr_common_stub

;  16: Coprocessor Fault Exception
isr16:
    cli
    push byte 0
    push byte 16
    jmp isr_common_stub

;  17: Alignment Check Exception
isr17:
    cli
    push byte 0
    push byte 17
    jmp isr_common_stub

;  18: Machine Check Exception
isr18:
    cli
    push byte 0
    push byte 18
    jmp isr_common_stub

; Now we only have reserved interrupts left, 19-31.
;  19: Reserved Interrupt
isr19:
    cli
    push byte 0
    push byte 19
    jmp isr_common_stub

;  20: Reserved Interrupt
isr20:
    cli
    push byte 0
    push byte 20
    jmp isr_common_stub

;  21: Reserved Interrupt
isr21:
    cli
    push byte 0
    push byte 21
    jmp isr_common_stub

;  22: Reserved Interrupt
isr22:
    cli
    push byte 0
    push byte 22
    jmp isr_common_stub

;  23: Reserved Interrupt
isr23:
    cli
    push byte 0
    push byte 23
    jmp isr_common_stub

;  24: Reserved Interrupt
isr24:
    cli
    push byte 0
    push byte 24
    jmp isr_common_stub

;  25: Reserved Interrupt
isr25:
    cli
    push byte 0
    push byte 25
    jmp isr_common_stub

;  26: Reserved Interrupt
isr26:
    cli
    push byte 0
    push byte 26
    jmp isr_common_stub

;  27: Reserved Interrupt
isr27:
    cli
    push byte 0
    push byte 27
    jmp isr_common_stub

;  28: Reserved Interrupt
isr28:
    cli
    push byte 0
    push byte 28
    jmp isr_common_stub

;  29: Reserved Interrupt
isr29:
    cli
    push byte 0
    push byte 29
    jmp isr_common_stub

;  30: Reserved Interrupt
isr30:
    cli
    push byte 0
    push byte 30
    jmp isr_common_stub

; 31: Reserved Interrupt
isr31:
    cli
    push byte 0
    push byte 31
    jmp isr_common_stub

; We call a C function in here. We need to let the assembler know
; that '_fault_handler' exists in another file
extern fault_handler

; This is our common ISR stub. It saves the processor state, sets
; up for kernel mode segments, calls the C-level fault handler,
; and finally restores the stack frame.
isr_common_stub:
    pusha
    push ds
    push es
    push fs
    push gs
    mov ax, 0x10   ; Load the Kernel Data Segment descriptor!
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov eax, esp   ; Push us the stack
    push eax
    mov eax, fault_handler
    call eax       ; A special call, preserves the 'eip' register
    pop eax
    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 8     ; Cleans up the pushed error code and pushed ISR number
    iret           ; pops 5 things at once: CS, EIP, EFLAGS, SS, and ESP!