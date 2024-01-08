#ifndef _MEMORY_H
#define _MEMORY_H

#include "inc_c/multiboot.h"

void memory_initialize(multiboot_info_t *mboot_info);
void *kmalloc(uint32_t size, bool align);
void kfree(void *ptr);
void heap_dump();

#endif