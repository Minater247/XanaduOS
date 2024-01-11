#ifndef _MEMORY_H
#define _MEMORY_H

#include <stdint.h>
#include <stdbool.h>

#include "inc_c/multiboot.h"

typedef struct {
    uint32_t pd_entry;
    uint32_t pt_entry;
} __attribute__((packed)) page_table_entry_t;

void memory_initialize(multiboot_info_t *mboot_info);
void *kmalloc(uint32_t size);
void *kmalloc_a(uint32_t size);
void *kmalloc_p(uint32_t size, uint32_t *phys);
void *kmalloc_ap(uint32_t size, uint32_t *phys);
void kfree(void *ptr);
void heap_dump();
page_table_entry_t first_free_page();
void alloc_page(uint32_t virt, uint32_t phys, bool make, bool is_kernel, bool is_writeable);

#endif