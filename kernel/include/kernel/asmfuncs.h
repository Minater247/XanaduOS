#ifndef _ASMFUNCS_H_
#define _ASMFUNCS_H_

#include <stdint.h>
#include "../../arch/i386/asmfuncs.c" //the linker fails if I don't do this so I guess this is what we do now!

inline void outb(uint16_t port, uint8_t val);
inline uint8_t inb(uint16_t port);

#endif