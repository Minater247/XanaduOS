#ifndef _ASMFUNCS_H_
#define _ASMFUNCS_H_

#include <stdint.h>
#include "../../arch/i386/asmfuncs.c" //This is the only one that doesn't work without this. Don't know why. Temporary fix until we need to port to different architectures.

inline void outb(uint16_t port, uint8_t val);
inline uint8_t inb(uint16_t port);
inline int check_CPUID();
inline void get_processor_vendor(char *vendor);
inline int long_mode_available();
inline int check_a20_gate();
inline void loadPageDirectory(uint32_t* inp);
inline void enablePaging();
inline void disablePaging();
inline void enablePAE();
inline void disablePAE();

#endif