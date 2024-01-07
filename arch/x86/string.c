#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

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

//itoa (x86)
//likely to later be moved to non-arch specific, since this uses no x86 specific instructions
char* itoa(int num, char *buffer, int base) {
    int i = 0;
    bool isNegative = false;
 
    if (num == 0) {
        buffer[i++] = '0';
        buffer[i] = '\0';
        return buffer;
    }
 
    if (num < 0 && base == 10) {
        isNegative = true;
        num = -num;
    } else if (base == 16) {
        //we need to do the whole thing unsigned
        uint32_t unum = (uint32_t)num;

        while (unum != 0) {
            int rem = unum % base;
            buffer[i++] = (rem > 9) ? (rem-10) + 'A' : rem + '0';
            unum = unum/base;
        }

        buffer[i] = '\0';

        int start = 0;
        int end = i - 1;
        char temp;
        for (start = 0; start < end; start++, end--) {
            temp = *(buffer+start);
            *(buffer+start) = *(buffer+end);
            *(buffer+end) = temp;
        }

        return buffer;
    }

    while (num != 0) {
        int rem = num % base;
        buffer[i++] = (rem > 9) ? (rem-10) + 'A' : rem + '0';
        num = num/base;
    }

    if (isNegative) {
        buffer[i++] = '-';
    }

    buffer[i] = '\0';

    int start = 0;
    int end = i - 1;
    char temp;
    for (start = 0; start < end; start++, end--) {
        temp = *(buffer+start);
        *(buffer+start) = *(buffer+end);
        *(buffer+end) = temp;
    }

    return buffer;
}