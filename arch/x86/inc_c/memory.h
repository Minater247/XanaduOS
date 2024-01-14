#ifndef _MEMORY_H
#define _MEMORY_H

#include <stdint.h>
#include <stdbool.h>

#include "inc_c/multiboot.h"

typedef struct {
    uint32_t pd_entry;
    uint32_t pt_entry;
} __attribute__((packed)) page_table_entry_t;

typedef struct heap_header
{
    uint32_t magic;
    uint32_t length;
    bool free;
    struct heap_header *next;
    struct heap_header *prev;
} __attribute__((packed)) heap_header_t;

typedef struct memory_region
{
    uint32_t start;
    uint32_t end;
    struct memory_region *next;
} __attribute__((packed)) memory_region_t;

typedef struct bitmap_1024
{
    uint32_t bitmap[32];
} __attribute__((packed)) bitmap_1024_t;

typedef struct {
    uint32_t pt_entry[1024];
} __attribute__((packed)) page_table_t;

typedef struct {
    uint32_t entries[1024];
    uint32_t virt[1024]; //virtual addresses of the page tables
    bool is_full[1024];
    uint32_t phys_addr;
} __attribute__((packed)) page_directory_t;

void memory_initialize(multiboot_info_t *mboot_info);
void *kmalloc(uint32_t size);
void *kmalloc_a(uint32_t size);
void *kmalloc_p(uint32_t size, uint32_t *phys);
void *kmalloc_ap(uint32_t size, uint32_t *phys);
void kfree(void *ptr);
void heap_dump();
page_table_entry_t first_free_page();
void alloc_page(uint32_t virt, uint32_t phys, bool make, bool is_kernel, bool is_writeable);
page_directory_t *clone_page_directory(page_directory_t *directory);
void switch_page_directory(page_directory_t *directory);

#endif