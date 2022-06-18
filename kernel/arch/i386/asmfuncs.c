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

/*

    Check whether the a20 gate is enabled


    Assembly code to use:

    pushad
    mov 0x112345, %edi
    mov 0x012345, %esi
    mov %esi, (%esi)
    mov %edi, (%edi)
    cmpsd
    popad
    jne a20_gate_not_enabled
    a20_gate_enabled:
    mov $1, %eax
    jmp a20_gate_done
    a20_gate_not_enabled:
    mov $0, %eax
    a20_gate_done:
    ret

*/
inline int check_a20_gate()
{
    uint32_t eax, ebx;
    asm volatile ( "pushal\n\t"
                   "mov 0x112345, %%edi\n\t"
                   "mov 0x012345, %%esi\n\t"
                   "mov %%esi, (%%esi)\n\t"
                   "mov %%edi, (%%edi)\n\t"
                   "cmpsd\n\t"
                   "popal\n\t"
                   "jne a20_gate_not_enabled\n\t"
                   "a20_gate_enabled:\n\t"
                   "mov $1, %%eax\n\t"
                   "jmp a20_gate_done\n\t"
                   "a20_gate_not_enabled:\n\t"
                   "mov $0, %%eax\n\t"
                   "a20_gate_done:"
                   : "=r"(eax), "=r"(ebx)
                   :
                   : "cc" );
    return eax;
}