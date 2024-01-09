#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "inc_c/multiboot.h"
#include "inc_c/display.h"
#include "inc_c/memory.h"
#include "inc_c/string.h"
#include "../../kernel/include/errors.h"
#include "../../kernel/include/unused.h"

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
    uint32_t phys_addr;
} __attribute__((packed)) page_directory_t;


// typedef struct {
//     uint32_t pd_entry;
//     uint32_t pt_entry;
// } __attribute__((packed)) page_table_entry_t;


#define HEAP_MAGIC 0xFEAF2004

extern uint8_t KERNEL_END; // defined in linker.ld
extern uint32_t page_directory[]; // defined in paging.s

heap_header_t *kheap = NULL;
uint32_t kheap_end = (uint32_t)&KERNEL_END;
#define INIT_MAP_END 0x800000 // 8 MB (set up in paging.s)
#define INIT_HEAP_SIZE 0x100000 // 1 MB (technically minus the heap header size)
memory_region_t *memory_map = NULL;
uint32_t total_mem_size = 0;

bitmap_1024_t *page_directory_bitmaps[1024]; // physical memory bitmap for memory allocation
page_directory_t kernel_pd __attribute__((aligned(4096))); // kernel page directory

void memory_initialize(multiboot_info_t *mboot_info)
{
    kassert_msg(mboot_info->flags & MULTIBOOT_INFO_MEM_MAP, "No memory map provided by bootloader.");

    multiboot_memory_map_t *mmap;
    for (mmap = (multiboot_memory_map_t *)mboot_info->mmap_addr;
         (uint32_t)mmap < mboot_info->mmap_addr + mboot_info->mmap_length;
         mmap = (multiboot_memory_map_t *)((uint32_t)mmap + mmap->size + sizeof(mmap->size)))
    {
        if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE)
        {
            terminal_printf("Usable memory: 0x%lx - 0x%lx\n", mmap->addr + 0xC0000000, mmap->addr + mmap->len + 0xC0000000);
            if (!memory_map)
            {
                memory_map = (memory_region_t *)kmalloc(sizeof(memory_region_t));
                memory_map->start = mmap->addr + 0xC0000000;
                memory_map->end = mmap->addr + mmap->len + 0xC0000000;
                memory_map->next = NULL;
            }
            else
            {
                memory_region_t *new_region = (memory_region_t *)kmalloc(sizeof(memory_region_t));
                new_region->start = mmap->addr + 0xC0000000;
                new_region->end = mmap->addr + mmap->len + 0xC0000000;
                new_region->next = memory_map;
                memory_map = new_region;
            }
        }

        total_mem_size += mmap->len;
    }

    // Print the memory map
    terminal_printf("Memory map:\n");
    memory_region_t *region = memory_map;
    while (region)
    {
        terminal_printf("0x%x - 0x%x\n", region->start, region->end);
        region = region->next;
    }
    terminal_printf("Total memory: %d MB\n", total_mem_size / 1024 / 1024);

    //it's time - clear first 2 entries in page directory
    page_directory[0] = 0;
    page_directory[1] = 0;

    //clear the page directory pointer table
    memset(page_directory_bitmaps, 0, sizeof(page_directory_bitmaps));
    //allocate the first 2 bitmaps and fill with 1
    page_directory_bitmaps[0] = (bitmap_1024_t *)kmalloc(sizeof(bitmap_1024_t));
    page_directory_bitmaps[1] = (bitmap_1024_t *)kmalloc(sizeof(bitmap_1024_t));

    memset(page_directory_bitmaps[0]->bitmap, 0xFF, sizeof(page_directory_bitmaps[0]->bitmap));
    memset(page_directory_bitmaps[1]->bitmap, 0xFF, sizeof(page_directory_bitmaps[1]->bitmap));

    //set up the kernel page table
    memset(kernel_pd.entries, 0, sizeof(kernel_pd.entries));
    //Presently set entries will be 0x300 -> 0 and 0x301 -> 0x400000
    uint32_t phys;
    page_table_t *new_map = (page_table_t *)kmalloc_ap(sizeof(page_table_t), &phys);
    memset(new_map->pt_entry, 0, sizeof(new_map->pt_entry));
    kernel_pd.entries[0x300] = phys | 0x3;
    kernel_pd.virt[0x300] = (uint32_t)new_map;
    new_map = (page_table_t *)kmalloc_ap(sizeof(page_table_t), &phys);
    memset(new_map->pt_entry, 0, sizeof(new_map->pt_entry));
    kernel_pd.entries[0x301] = phys | 0x3;
    kernel_pd.virt[0x301] = (uint32_t)new_map;

    //set up the page tables for the first 8MB
    for (uint32_t i = 0; i < 1024; i++)
    {
        ((uint32_t *)kernel_pd.virt[0x300])[i] = (i * 0x1000) | 0x3;
        ((uint32_t *)kernel_pd.virt[0x301])[i] = (i * 0x1000 + 0x400000) | 0x3;
    }

    //switch to the new page directory
    asm volatile("mov %0, %%cr3":: "r"((uint32_t)&kernel_pd - 0xC0000000));

    //check whether we already have enough space after the heap
    if (kheap_end + INIT_HEAP_SIZE > INIT_MAP_END + 0xC0000000) {
        //we need another page table
        page_table_entry_t entry = first_free_page();
        page_table_t *new_map = (page_table_t *)kmalloc_ap(sizeof(page_table_t), &phys);
        memset(new_map->pt_entry, 0, sizeof(new_map->pt_entry));
        kernel_pd.entries[entry.pd_entry + 0x300] = phys | 0x3;
        kernel_pd.virt[entry.pd_entry + 0x300] = (uint32_t)new_map;
        //set up the page table
        for (uint32_t i = 0; i < 1024; i++)
        {
            ((uint32_t *)kernel_pd.virt[entry.pd_entry + 0x300])[i] = (i * 0x1000 + entry.pd_entry * 0x400000) | 0x3;
        }
        //set up the bitmap
        page_directory_bitmaps[entry.pd_entry] = (bitmap_1024_t *)kmalloc(sizeof(bitmap_1024_t));
        memset(page_directory_bitmaps[entry.pd_entry]->bitmap, 0xFF, sizeof(page_directory_bitmaps[entry.pd_entry]->bitmap));
        terminal_printf("Mapped memory expanded to 12MB\n");
    }

    //set up the heap
    kheap = (heap_header_t *)kheap_end;
    kheap->magic = HEAP_MAGIC;
    kheap->length = INIT_HEAP_SIZE - sizeof(heap_header_t);
    kheap->free = true;
    kheap->next = NULL;
    kheap->prev = NULL;
    kheap_end += INIT_HEAP_SIZE;

    page_table_entry_t entry = first_free_page();
    terminal_printf("First free page: %d, %d\n", entry.pd_entry, entry.pt_entry);

    //allocate and deallocate some stuff to test the heap
    uint32_t *test = (uint32_t *)kmalloc(4);
    kassert(test != NULL);
    *test = 0xDEADBEEF;
    kassert(*test == 0xDEADBEEF);
    kfree(test);
    kassert(kheap->length == INIT_HEAP_SIZE - sizeof(heap_header_t));
    terminal_printf("\033[1;32mHeap test passed!\033[0m\n");

    //allocate 1MB of memory to test heap
    test = (uint32_t *)kmalloc(1024 * 1024);
    kassert(test != NULL); //make sure we can expand the heap (this is sizeof(heap_header_t) too big or an allocation, so it will require expansion)
}


page_table_entry_t first_free_page()
{
    page_table_entry_t entry;
    entry.pd_entry = 0;
    entry.pt_entry = 0;
    bool found = false;
    for (uint32_t i = 0; i < 1024; i++)
    {
        if (page_directory_bitmaps[i] == NULL)
        {
            entry.pd_entry = i;
            entry.pt_entry = 0;
            found = true;
            break;
        }
        else
        {
            for (uint32_t j = 0; j < 32; j++)
            {
                //find the first free bit if the pte bitmap is not full
                if (page_directory_bitmaps[i]->bitmap[j] != 0xFFFFFFFF)
                {
                    for (uint32_t k = 0; k < 32; k++)
                    {
                        if ((page_directory_bitmaps[i]->bitmap[j] & (1 << k)) == 0)
                        {
                            entry.pd_entry = i;
                            entry.pt_entry = j * 32 + k;
                            found = true;
                            break;
                        }
                    }
                }
                if (found)
                {
                    break;
                }
            }
        }
        if (found)
        {
            break;
        }
    }
    if (!found)
    {
        kpanic("No free pages available!");
    }
    return entry;
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

void *kmalloc_int(uint32_t size, bool align, uint32_t *phys)
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
        if (phys != NULL)
        {
            *phys = old_end - 0xC0000000;
        }
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
                
                if (phys != NULL)
                {
                    kpanic("kmalloc with align and phys not implemented");
                }

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

                if (phys != NULL)
                {
                    kpanic("kmalloc with phys not implemented");
                }

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

void *kmalloc_a(uint32_t size)
{
    return kmalloc_int(size, true, NULL);
}

void *kmalloc_p(uint32_t size, uint32_t *phys)
{
    return kmalloc_int(size, false, phys);
}

void *kmalloc_ap(uint32_t size, uint32_t *phys) {
    return kmalloc_int(size, true, phys);
}

void *kmalloc(uint32_t size) {
    return kmalloc_int(size, false, NULL);
}