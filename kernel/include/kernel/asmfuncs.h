#ifndef _ASMFUNCS_H_
#define _ASMFUNCS_H_

#include <stdint.h>
#include "../../arch/i386/asmfuncs.c" //temporary hack until we need to port this to other architectures (include wasn't working for an unknown reason)

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