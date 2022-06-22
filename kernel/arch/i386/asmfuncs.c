#include <stdint.h>

inline void outb(uint16_t port, uint8_t val)
{
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
    /* There's an outb %al, $imm8  encoding, for compile-time constant port numbers that fit in 8b.  (N constraint).
     * Wider immediate constants would be truncated at assemble-time (e.g. "i" constraint).
     * The  outb  %al, %dx  encoding is the only option for all other cases.
     * %1 expands to %dx because  port  is a uint16_t.  %w1 could be used if we had the port number a wider C type */
}
inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    asm volatile ( "inb %1, %0"
                   : "=a"(ret)
                   : "Nd"(port) );
    return ret;
}

//Checks whether CPUID is available
inline int check_CPUID()
{
    uint32_t eax, ebx, ecx, edx;
    __asm__ __volatile__ (
        "movl $0, %%eax\n\t"
        "cpuid\n\t"
        "movl %%ebx, %1\n\t"
        "movl %%ecx, %2\n\t"
        "movl %%edx, %3"
        : "=a"(eax), "=r"(ebx), "=r"(ecx), "=r"(edx)
        :
        : "cc"
    );
    return (ebx != 0);
}

inline void cpuid(uint32_t op, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx)
{
    asm volatile ( "cpuid"
                   : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
                   : "a"(op)
                   : "cc" );
}

//Get processor vendor
inline void get_processor_vendor(char *vendor)
{
    uint32_t eax, ebx, ecx, edx;
    uint32_t op = 0;
    cpuid(op, &eax, &ebx, &ecx, &edx);
    //Vendor is in ebx, edx, ecx
    vendor[0] = ebx & 0xFF;
    vendor[1] = (ebx >> 8) & 0xFF;
    vendor[2] = (ebx >> 16) & 0xFF;
    vendor[3] = (ebx >> 24) & 0xFF;
    vendor[4] = edx & 0xFF;
    vendor[5] = (edx >> 8) & 0xFF;
    vendor[6] = (edx >> 16) & 0xFF;
    vendor[7] = (edx >> 24) & 0xFF;
    vendor[8] = ecx & 0xFF;
    vendor[9] = (ecx >> 8) & 0xFF;
    vendor[10] = (ecx >> 16) & 0xFF;
    vendor[11] = (ecx >> 24) & 0xFF;
    vendor[12] = '\0';
}

inline int long_mode_available()
{
    uint32_t eax, ebx, ecx, edx;
    uint32_t op = 0x80000000;
    cpuid(op, &eax, &ebx, &ecx, &edx);
    return (eax >= 0x80000001);
}

/* This defines what the stack looks like after an ISR was running */
struct regs
{
    unsigned int gs, fs, es, ds;      /* pushed the segs last */
    unsigned int edi, esi, ebp, esp, ebx, edx, ecx, eax;  /* pushed by 'pusha' */
    unsigned int int_no, err_code;    /* our 'push byte #' and ecodes do this */
    unsigned int eip, cs, eflags, useresp, ss;   /* pushed by the processor automatically */ 
};

/*

    Check whether the a20 gate is enabled

    The a20 gate is disabled if the ax register is 0, enabled if it is 1.

    Assembly code to use:

    .intel_syntax noprefix
    check_a20:
        pushf
        push ds
        push es
        push di
        push si
    
        cli
    
        xor ax, ax ; ax = 0
        mov es, ax
    
        not ax ; ax = 0xFFFF
        mov ds, ax
    
        mov di, 0x0500
        mov si, 0x0510
    
        mov al, byte [es:di]
        push ax
    
        mov al, byte [ds:si]
        push ax
    
        mov byte [es:di], 0x00
        mov byte [ds:si], 0xFF
    
        cmp byte [es:di], 0xFF
    
        pop ax
        mov byte [ds:si], al
    
        pop ax
        mov byte [es:di], al
    
        mov ax, 0
        je check_a20__exit
    
        mov ax, 1
 
    check_a20__exit:
        pop si
        pop di
        pop es
        pop ds
        popf
    
        ret

*/
//currently broken
inline int check_a20_gate()
{
    uint32_t eax, ebx, ecx, edx;
    asm volatile (
        ".intel_syntax noprefix\n\t"
        "pushf\n\t"
        "push ds\n\t"
        "push es\n\t"
        "push di\n\t"
        "push si\n\t"
        "xor ax, ax\n\t" 
        "mov es, ax\n\t"
        "not ax\n\t"
        "mov ds, ax\n\t"
        "xor ax, ax\n\t"
        "mov di, 0x500\n\t"
        "mov si, 0x510\n\t" 
        "mov al, byte [es:0x500]\n\t"
        "push ax\n\t"
        "mov al, byte [ds:0x510]\n\t"
        "push ax\n\t"
        "movb byte [es:0x500], 0x00\n\t" 
        "movb byte [ds:0x510], 0xFF\n\t" 
        "cmpb byte [es:0x500], 0xFF\n\t" 
        "pop ax\n\t"
        "mov byte [ds:0x510], al\n\t"
        "pop ax\n\t"
        "mov byte [es:0x500], al\n\t"
        "mov ax, 0\n\t"
        "je check_a20__exit\n\t"
        "mov ax, 1\n\t"
        "check_a20__exit:\n\t"
        "pop si\n\t"
        "pop di\n\t"
        "pop es\n\t"
        "pop ds\n\t"
        "popf\n\t"
        ".att_syntax prefix"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        :
        : "cc"
    );
    return eax;
}

//load page directory
/*
    push %ebp
    mov %esp, %ebp
    mov 8(%esp), %eax
    mov %eax, %cr3
    mov %ebp, %esp
    pop %eb
*/
inline void loadPageDirectory(uint32_t* inp) {
    asm volatile (
        "push %%ebp\n\t"
        "mov %%esp, %%ebp\n\t"
        "mov 8(%%esp), %%eax\n\t"
        "mov %%eax, %%cr3\n\t"
        "mov %%ebp, %%esp\n\t"
        "pop %%ebp"
        :
        : "a"(inp)
        : "cc"
    );  //inp is the address of the page directory
}

//enable paging
/*
    push %ebp
    mov %esp, %ebp
    mov %cr0, %eax
    or $0x80000000, %eax
    mov %eax, %cr0
    mov %ebp, %esp
    pop %ebp
*/
inline void enablePaging() {
    asm volatile (
        "push %%ebp\n\t"
        "mov %%esp, %%ebp\n\t"
        "mov %%cr0, %%eax\n\t"
        "or $0x80000000, %%eax\n\t"
        "mov %%eax, %%cr0\n\t"
        "mov %%ebp, %%esp\n\t"
        "pop %%ebp"
        :
        :
        : "cc"
    );
}

//disable paging by setting cr0.pg to 0
/*
    push %ebp
    mov %esp, %ebp
    mov %cr0, %eax
    and $0x7FFFFFFF, %eax
    mov %eax, %cr0
    mov %ebp, %esp
    pop %ebp
*/
inline void disablePaging() {
    asm volatile (
        "push %%ebp\n\t"
        "mov %%esp, %%ebp\n\t"
        "mov %%cr0, %%eax\n\t"
        "and $0x7FFFFFFF, %%eax\n\t"
        "mov %%eax, %%cr0\n\t"
        "mov %%ebp, %%esp\n\t"
        "pop %%ebp"
        :
        :
        : "cc"
    );
}

//enable PAE in cr5.pae (bit 5)
/*
    push %ebp
    mov %esp, %ebp
    mov %cr4, %eax
    or $0x20, %eax
    mov %eax, %cr4
    mov %ebp, %esp
    pop %ebp
*/
inline void enablePAE() {
    asm volatile (
        "push %%ebp\n\t"
        "mov %%esp, %%ebp\n\t"
        "mov %%cr4, %%eax\n\t"
        "or $0x20, %%eax\n\t"
        "mov %%eax, %%cr4\n\t"
        "mov %%ebp, %%esp\n\t"
        "pop %%ebp"
        :
        :
        : "cc"
    );
}

//disable PAE in cr5.pae (bit 5)
/*
    push %ebp
    mov %esp, %ebp
    mov %cr4, %eax
    and $0xFFFFFFDF, %eax
    mov %eax, %cr4
    mov %ebp, %esp
    pop %ebp
*/
inline void disablePAE() {
    asm volatile (
        "push %%ebp\n\t"
        "mov %%esp, %%ebp\n\t"
        "mov %%cr4, %%eax\n\t"
        "and $0xFFFFFFDF, %%eax\n\t"
        "mov %%eax, %%cr4\n\t"
        "mov %%ebp, %%esp\n\t"
        "pop %%ebp"
        :
        :
        : "cc"
    );
}