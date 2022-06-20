#ifndef _GDT_H
#define _GDT_H 1

#include <stdint.h>
#include "../../arch/i386/GDT.c"

void gdt_install();

#endif