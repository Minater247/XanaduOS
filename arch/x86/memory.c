#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "inc_c/multiboot.h"
#include "inc_c/display.h"
#include "inc_c/memory.h"
#include "../../kernel/include/errors.h"

typedef struct heap_header
{
    uint32_t magic;
    uint32_t length;
    bool free;
    struct heap_header *next;
    struct heap_header *prev;
} __attribute__((packed)) heap_header_t;

#define HEAP_MAGIC 0xFEAF2004

extern uint8_t KERNEL_END; // defined in linker.ld

heap_header_t *kheap = NULL;
uint32_t kheap_end = (uint32_t)&KERNEL_END;

void memory_initialize(multiboot_info_t *mboot_info)
{
    kassert_msg(mboot_info->flags & MULTIBOOT_INFO_MEM_MAP, "No memory map provided by bootloader.");

    heap_header_t *last_header = NULL;

    multiboot_memory_map_t *mmap;
    for (mmap = (multiboot_memory_map_t *)mboot_info->mmap_addr;
         (uint32_t)mmap < mboot_info->mmap_addr + mboot_info->mmap_length;
         mmap = (multiboot_memory_map_t *)((uint32_t)mmap + mmap->size + sizeof(mmap->size)))
    {
        if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE)
        {
            // assign a header to this memory block if it's above the end of the kernel
            uint32_t header_start_loc = mmap->addr + 0xC0000000;
            uint32_t header_end_loc = mmap->addr + mmap->len + 0xC0000000;

            if (header_start_loc < kheap_end)
            {
                header_start_loc = kheap_end;
            }

            if ((header_start_loc < header_end_loc) && (header_end_loc - header_start_loc > sizeof(heap_header_t)))
            {
                heap_header_t *header = (heap_header_t *)header_start_loc;
                header->magic = HEAP_MAGIC;
                header->length = header_end_loc - header_start_loc - sizeof(heap_header_t);
                header->next = NULL;
                header->prev = last_header;
                header->free = true;

                if (last_header != NULL)
                {
                    last_header->next = header;
                }

                last_header = header;

                if (kheap == NULL)
                {
                    kheap = header;
                }
            }
        }
    }

    kassert_msg(kheap != NULL, "No available memory for heap.");

    uint32_t total_heap_mem = 0;
    for (heap_header_t *header = kheap; header != NULL; header = header->next)
    {
        total_heap_mem += header->length;
    }
    terminal_printf("Total heap size: %d bytes, %d KB, %d MB\n", total_heap_mem, total_heap_mem / 1024, total_heap_mem / 1024 / 1024);
}

void heap_dump()
{
    terminal_printf("Heap dump:\n");
    // for each header, print the header, the contents, and the next header
    for (heap_header_t *header = kheap; header != NULL; header = header->next)
    {
        terminal_printf("@ 0x%x, length: %d\n", header, header->length);
        terminal_printf("Contents: 0x%x\n", *(uint32_t *)((uint32_t)header + sizeof(heap_header_t)));
    }
}

void *kmalloc(uint32_t size, bool align)
{
    if (kheap == NULL)
    {
        // simple alloc
        if (align)
        {
            kheap_end = (kheap_end & 0xFFF) ? (kheap_end & 0xFFFFF000) + 0x1000 : kheap_end;
        }
        uint32_t old_end = kheap_end;
        kheap_end += size;
        return (void *)old_end;
    }
    if (align)
    {
        heap_header_t *header;
        for (header = kheap; header != NULL; header = header->next)
        {
            uint32_t aligned_addr = ((uint32_t)header & 0xFFF) ? ((uint32_t)header & 0xFFFFF000) + 0x1000 : (uint32_t)header;
            if (((aligned_addr + size) <= ((uint32_t)header + header->length)) && header->free)
            {
                // The aligned address block is within the header, and the header is free

                // Check if there is space before the aligned address block for another header
                if (aligned_addr > (uint32_t)header + (2 * sizeof(heap_header_t)))
                {
                    // this allocation needs a header
                    heap_header_t *new_header = (heap_header_t *)(aligned_addr - sizeof(heap_header_t));
                    new_header->magic = HEAP_MAGIC;
                    new_header->length = (uint32_t)header + header->length - aligned_addr;
                    new_header->next = header->next;
                    new_header->prev = header;
                    new_header->free = false;

                    if (header->next != NULL)
                    {
                        header->next->prev = new_header;
                    }

                    header->next = new_header;
                    header->length = aligned_addr - (uint32_t)header - (2 * sizeof(heap_header_t));
                    header = new_header;
                }

                // check if there is space after the aligned address block for another header
                if ((aligned_addr + size) < ((uint32_t)header + sizeof(heap_header_t) + header->length - sizeof(heap_header_t)))
                {
                    // there needs to be a header after this allocation
                    heap_header_t *new_header = (heap_header_t *)(aligned_addr + size);
                    new_header->magic = HEAP_MAGIC;
                    new_header->length = (uint32_t)header + sizeof(heap_header_t) + header->length - aligned_addr - size - sizeof(heap_header_t);
                    new_header->next = header->next;
                    new_header->prev = header;
                    new_header->free = true;

                    if (header->next != NULL)
                    {
                        header->next->prev = new_header;
                    }

                    header->next = new_header;
                    header->length = aligned_addr + size - (uint32_t)header - sizeof(heap_header_t);
                }

                header->free = false;

                // now we can return the aligned address
                return (void *)aligned_addr;
            }
        }
    }
    else
    {
        heap_header_t *header;
        for (header = kheap; header != NULL; header = header->next)
        {
            if (header->length >= size && header->free)
            {
                // we found a header that's big enough
                // now we need to split it if it's too big
                if (header->length > size + sizeof(heap_header_t))
                {
                    // split the header
                    heap_header_t *new_header = (heap_header_t *)((uint32_t)header + sizeof(heap_header_t) + size);
                    new_header->magic = HEAP_MAGIC;
                    new_header->length = header->length - size - sizeof(heap_header_t);
                    new_header->next = header->next;
                    new_header->prev = header;
                    new_header->free = true;

                    if (header->next != NULL)
                    {
                        header->next->prev = new_header;
                    }

                    header->next = new_header;
                    header->length = size;
                }

                header->free = false;

                // now we can return the header
                return (void *)((uint32_t)header + sizeof(heap_header_t));
            }
        }
    }
    // if we get here, we couldn't find a header that was big enough
    return NULL;
}

void kfree(void *ptr)
{
    kassert_msg(kheap != NULL, "kfree called before kheap initialization!");
    heap_header_t *header = (heap_header_t *)((uint32_t)ptr - sizeof(heap_header_t));
    kassert_msg(header->magic == HEAP_MAGIC, "Invalid heap header magic number.");
    header->free = true;
    // now we need to merge this header with the next header if it's free
    if (header->next != NULL && header->next->free)
    {
        // make sure the next header is directly after this one (may have memory holes)
        if ((uint32_t)header + header->length + sizeof(heap_header_t) != (uint32_t)header->next)
        {
            // we can't merge these headers
            return;
        }
        else
        {
            header->length += header->next->length + sizeof(heap_header_t);
            header->next = header->next->next;
            if (header->next != NULL)
            {
                header->next->prev = header;
            }
        }
    }
    // now we need to merge this header with the previous header if it's free
    if (header->prev != NULL && header->prev->free)
    {
        // make sure the previous header is directly before this one (may have memory holes)
        if ((uint32_t)header->prev + header->prev->length + sizeof(heap_header_t) != (uint32_t)header)
        {
            // we can't merge these headers
            return;
        }
        header->prev->length += header->length + sizeof(heap_header_t);
        header->prev->next = header->next;
        if (header->next != NULL)
        {
            header->next->prev = header->prev;
        }
    }
}