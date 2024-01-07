#include <stdint.h>
#include <stddef.h>

//memset (x86)
#define __HAVE_ARCH_MEMSET
void *memset(void *s, int c, size_t n)
{
    //use stosb to set byte by byte
    asm volatile("cld; rep stosb"
                 : "+D"(s), "+c"(n)
                 : "a"(c)
                 : "memory");
    return s;
}