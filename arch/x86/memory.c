#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "inc_c/multiboot.h"
#include "inc_c/display.h"
#include "inc_c/memory.h"
#include "inc_c/string.h"
#include "inc_c/serial.h"
#include "../../kernel/include/errors.h"
#include "../../kernel/include/unused.h"

#define HEAP_MAGIC 0xFEAF2004

extern uint8_t KERNEL_END; // defined in linker.ld
extern uint32_t page_directory[]; // defined in paging.s

heap_header_t *kheap = NULL;
uint32_t kheap_end = (uint32_t)&KERNEL_END;
#define INIT_MAP_END 0x800000 // 8 MB (set up in paging.s)
memory_region_t *memory_map = NULL;
uint32_t total_mem_size = 0;

bitmap_1024_t *page_directory_bitmaps[1024]; // physical memory bitmap for memory allocation
page_directory_t kernel_pd __attribute__((aligned(4096))); // kernel page directory
page_directory_t *current_pd = &kernel_pd; // current page directory

page_table_t *prealloc_table = NULL; // page table for preallocated memory
uint32_t prealloc_phys;
bitmap_1024_t *prealloc_bitmap = NULL; // bitmap for preallocated memory

void memory_initialize(multiboot_info_t *mboot_info)
{
    kassert_msg(mboot_info->flags & MULTIBOOT_INFO_MEM_MAP, "No memory map provided by bootloader.");

    //look for the ramdisk in modules to ensure we don't overwrite it
    multiboot_module_t *module;
    for (module = (multiboot_module_t *)mboot_info->mods_addr;
         (uint32_t)module < mboot_info->mods_addr + mboot_info->mods_count * sizeof(multiboot_module_t);
         module = (multiboot_module_t *)((uint32_t)module + sizeof(multiboot_module_t)))
    {
        if (strcmp((char *)module->cmdline, "RAMDISK") == 0)
        {
            if (module->mod_end > kheap_end)
            {
                kheap_end = module->mod_end;
            }
        }
    }

    multiboot_memory_map_t *mmap;
    for (mmap = (multiboot_memory_map_t *)mboot_info->mmap_addr;
         (uint32_t)mmap < mboot_info->mmap_addr + mboot_info->mmap_length;
         mmap = (multiboot_memory_map_t *)((uint32_t)mmap + mmap->size + sizeof(mmap->size)))
    {
        if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE)
        {
            if (!memory_map)
            {
                memory_map = (memory_region_t *)kmalloc(sizeof(memory_region_t));
                memory_map->start = mmap->addr;
                memory_map->end = mmap->addr + mmap->len;
                memory_map->next = NULL;
            }
            else
            {
                memory_region_t *new_region = (memory_region_t *)kmalloc(sizeof(memory_region_t));
                new_region->start = mmap->addr;
                new_region->end = mmap->addr + mmap->len;
                new_region->next = memory_map;
                memory_map = new_region;
            }
        }

        total_mem_size += mmap->len;
    }

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
    memset(kernel_pd.virt, 0, sizeof(kernel_pd.virt));
    memset(kernel_pd.is_full, 0, sizeof(kernel_pd.is_full));
    //Presently set entries will be 0x300 -> 0 and 0x301 -> 0x400000
    uint32_t phys;
    page_table_t *new_map = (page_table_t *)kmalloc_ap(sizeof(page_table_t), &phys);
    memset(new_map->pt_entry, 0, sizeof(new_map->pt_entry));
    kernel_pd.entries[0x300] = phys | 0x3;
    kernel_pd.virt[0x300] = (uint32_t)new_map;
    kernel_pd.is_full[0x300] = true;
    new_map = (page_table_t *)kmalloc_ap(sizeof(page_table_t), &phys);
    memset(new_map->pt_entry, 0, sizeof(new_map->pt_entry));
    kernel_pd.entries[0x301] = phys | 0x3;
    kernel_pd.virt[0x301] = (uint32_t)new_map;
    kernel_pd.is_full[0x301] = true;

    //set up the page tables for the first 8MB
    for (uint32_t i = 0; i < 1024; i++)
    {
        ((uint32_t *)kernel_pd.virt[0x300])[i] = (i * 0x1000) | 0x3;
        ((uint32_t *)kernel_pd.virt[0x301])[i] = (i * 0x1000 + 0x400000) | 0x3;
    }

    //point 0xFFFFE000 and 0xFFFFF000 to NULL
    new_map = (page_table_t *)kmalloc_ap(sizeof(page_table_t), &phys);
    memset(new_map->pt_entry, 0, sizeof(new_map->pt_entry));
    kernel_pd.entries[0x3FF] = phys | 0x3;
    kernel_pd.virt[0x3FF] = (uint32_t)new_map;
    kernel_pd.is_full[0x3FF] = false;
    ((page_table_t *)kernel_pd.virt[0x3FF])->pt_entry[1022] = 3;
    ((page_table_t *)kernel_pd.virt[0x3FF])->pt_entry[1023] = 3;

    //switch to the new page directory
    asm volatile("mov %0, %%cr3":: "r"((uint32_t)&kernel_pd - 0xC0000000));

    //check whether we already have enough space after the heap
    //Expansion should be fine in this case, since we are guaranteed to have through 16MB of DRAM
    if (kheap_end + 0x100000 > INIT_MAP_END + 0xC0000000) {
        //we need another page table
        page_table_entry_t entry = first_free_page();
        page_table_t *new_map = (page_table_t *)kmalloc_ap(sizeof(page_table_t), &phys);
        memset(new_map->pt_entry, 0, sizeof(new_map->pt_entry));
        kernel_pd.entries[entry.pd_entry + 0x300] = phys | 0x3;
        kernel_pd.virt[entry.pd_entry + 0x300] = (uint32_t)new_map;
        kernel_pd.is_full[entry.pd_entry + 0x300] = true;
        //set up the page table
        for (uint32_t i = 0; i < 1024; i++)
        {
            ((uint32_t *)kernel_pd.virt[entry.pd_entry + 0x300])[i] = (i * 0x1000 + entry.pd_entry * 0x400000) | 0x3;
        }
        //set up the bitmap
        page_directory_bitmaps[entry.pd_entry] = (bitmap_1024_t *)kmalloc(sizeof(bitmap_1024_t));
        memset(page_directory_bitmaps[entry.pd_entry]->bitmap, 0xFF, sizeof(page_directory_bitmaps[entry.pd_entry]->bitmap));
    }

    kernel_pd.phys_addr = (uint32_t)&kernel_pd - 0xC0000000;

    prealloc_table = (page_table_t *)kmalloc_ap(sizeof(page_table_t), &prealloc_phys);
    memset(prealloc_table->pt_entry, 0, sizeof(prealloc_table->pt_entry));
    prealloc_bitmap = (bitmap_1024_t *)kmalloc(sizeof(bitmap_1024_t));
    memset(prealloc_bitmap->bitmap, 0, sizeof(prealloc_bitmap->bitmap));

    //set up the heap
    kheap = (heap_header_t *)kheap_end;
    kheap->magic = HEAP_MAGIC;
    //fill the rest of memory, up to either 8MB ot 12MB
    if (kheap_end + 0x100000 > INIT_MAP_END + 0xC0000000) {
        kheap_end = 0xC0B00000;
        kheap->length = kheap_end - (uint32_t)kheap - sizeof(heap_header_t);
    } else {
        kheap_end = 0xC0800000;
        kheap->length = kheap_end - (uint32_t)kheap - sizeof(heap_header_t);
    }
    kheap->free = true;
    kheap->next = NULL;
    kheap->prev = NULL;
}



bool is_in_usable_memory(uint32_t phys_addr)
{
    memory_region_t *region = memory_map;
    while (region)
    {
        if (phys_addr >= region->start && phys_addr < region->end)
        {
            return true;
        }
        region = region->next;
    }
    return false;
}

page_table_entry_t first_free_page()
{
    for (uint32_t i = 0; i < 1024; i++)
    {
        if (page_directory_bitmaps[i] == NULL)
        {
            memory_region_t *region = memory_map;
            while (region)
            {
                //check if it overlaps at all, wither with the end or start
                if (region->start < i * 1024 * 1024 + 1024 * 1024 && region->end > i * 1024 * 1024)
                {
                    return (page_table_entry_t){.pd_entry = i, .pt_entry = 0};
                }
                region = region->next;
            }
        } else if (current_pd->is_full[i]) {
            continue;
        }
        for (uint32_t j = 0; j < 32; j++)
        {
            if (page_directory_bitmaps[i]->bitmap[j] != 0xFFFFFFFF)
            {
                for (uint32_t k = 0; k < 32; k++)
                {
                    if (!is_in_usable_memory((i * 1024 * 1024) + (j * 32 + k) * 4096))
                    {
                        continue;
                    }
                    if ((page_directory_bitmaps[i]->bitmap[j] & (1 << k)) == 0)
                    {
                        return (page_table_entry_t){.pd_entry = i, .pt_entry = j * 32 + k};
                    }
                }
            }
        }
    }
    kpanic("No free pages available!");
}


void phys_copypage(uint32_t src, uint32_t dest) {
    ((page_table_t *)current_pd->virt[0x3FF])->pt_entry[1022] = (src & 0xFFFFF000) | 0x3;
    ((page_table_t *)current_pd->virt[0x3FF])->pt_entry[1023] = (dest & 0xFFFFF000) | 0x3;
    
    memcpy((void *)0xFFFFE000, (void *)0xFFFFF000, 4096);
}


page_directory_t *clone_page_directory(page_directory_t *directory) {
    uint32_t phys;
    page_directory_t *new_directory = (page_directory_t *)kmalloc_ap(sizeof(page_directory_t), &phys);
    memset(new_directory, 0, sizeof(page_directory_t));
    new_directory->phys_addr = phys;

    for (uint32_t pde = 0; pde < 1024; pde++) {
        if (directory->entries[pde] & 0x1) {
            new_directory->virt[pde] = (uint32_t)kmalloc_ap(sizeof(page_table_t), &phys);
            new_directory->entries[pde] = phys | 0x3;
            new_directory->is_full[pde] = directory->is_full[pde];

            for (uint32_t pte = 0; pte < 1024; pte++) {
                if (((page_table_t *)directory->virt[pde])->pt_entry[pte] & 0x1) {
                    //if it's the same as the kernel, skip it
                    uint32_t kernel_entry = ((page_table_t *)kernel_pd.virt[pde])->pt_entry[pte] & 0xFFFFF000;
                    uint32_t this_entry = ((page_table_t *)directory->virt[pde])->pt_entry[pte] & 0xFFFFF000;

                    if (kernel_entry == this_entry) {
                        ((page_table_t *)new_directory->virt[pde])->pt_entry[pte] = this_entry | 0x3;
                        continue;
                    }

                    //page_table_entry_t entry = first_free_page();

                    kpanic("Observe this code before continuing!");
                }
            }
        }
    }

    return new_directory;
}

void free_page_directory(page_directory_t *directory) {
    //if the pte is the same as the kernel, don't free it
    for (uint32_t pde = 0; pde < 1024; pde++) {
        if (directory->entries[pde] & 0x1) {
            for (uint32_t pte = 0; pte < 1024; pte++) {
                if (((page_table_t *)directory->virt[pde])->pt_entry[pte] & 0x1) {
                    if (kernel_pd.virt[pde] & 0x1) {
                        if (((page_table_t *)directory->virt[pde])->pt_entry[pte] == ((page_table_t *)kernel_pd.virt[pde])->pt_entry[pte]) {
                            continue;
                        }
                    }
                    uint32_t phys = ((page_table_t *)directory->virt[pde])->pt_entry[pte] & 0xFFFFF000;
                    uint32_t pd_entry = phys >> 22;
                    uint32_t pt_entry = (phys >> 12) & 0x3FF;
                    page_directory_bitmaps[pd_entry]->bitmap[pt_entry / 32] &= ~(1 << (pt_entry % 32));
                }
            }
            kfree_a((void *)directory->virt[pde]);
        }
    }
    //free the directory
    kfree(directory);
}


void switch_page_directory(page_directory_t *directory) {
    //bochs magic breakpoint
    current_pd = directory;
    asm volatile("mov %0, %%cr3":: "r"(directory->phys_addr));
}


//allocate a page of memory
void alloc_page(uint32_t virt, uint32_t phys, bool make, bool is_kernel, bool is_writeable) {
    //get the page directory entry
    uint32_t pd_entry = virt >> 22;
    uint32_t pt_entry = (virt >> 12) & 0x3FF;

    phys &= 0xFFFFF000; //make sure the physical address is page-aligned

    //check if the page is already in use
    if (current_pd->virt[pd_entry] == 0 && make) {
        kassert(prealloc_table != NULL);
        //we already have a preallocated space for this page table
        current_pd->virt[pd_entry] = (uint32_t)prealloc_table;
        current_pd->entries[pd_entry] = prealloc_phys | 0x3;
        if (!is_kernel) {
            current_pd->entries[pd_entry] |= 0x4;
        }
        if (is_writeable) {
            current_pd->entries[pd_entry] |= 0x2;
        }
        current_pd->is_full[pd_entry] = false;
        prealloc_table = NULL;
    }
    if ((*(page_table_t *)(current_pd->virt[pd_entry])).pt_entry[pt_entry] & 0x1) {
        kpanic("Attempted to allocate already allocated page!");
    }

    //set the page table entry
    ((page_table_t *)(current_pd->virt[pd_entry]))->pt_entry[pt_entry] = phys | 0x3;

    //if the bitmap doesn't exist, use the preallocated one
    uint32_t phys_pd_entry = phys >> 22;
    uint32_t phys_pt_entry = (phys >> 12) & 0x3FF;
    if (page_directory_bitmaps[phys_pd_entry] == NULL) {
        page_directory_bitmaps[phys_pd_entry] = prealloc_bitmap;
        current_pd->is_full[phys_pd_entry] = false;
        prealloc_bitmap = NULL;
    }

    //set the bitmap
    page_directory_bitmaps[phys_pd_entry]->bitmap[phys_pt_entry / 32] |= (1 << (phys_pt_entry % 32));

    //check if the page directory entry is full
    for (uint32_t i = 0; i < 32; i++) {
        if (page_directory_bitmaps[phys_pd_entry]->bitmap[i] != 0xFFFFFFFF) {
            return;
        }
    }
    //the page directory entry is full
    current_pd->is_full[phys_pd_entry] = true;
    return;
}

void alloc_page_kmalloc(uint32_t virt, uint32_t phys, bool make, bool is_kernel, bool is_writeable) {
    //get the page directory entry
    uint32_t pd_entry = virt >> 22;
    uint32_t pt_entry = (virt >> 12) & 0x3FF;

    phys &= 0xFFFFF000; //make sure the physical address is page-aligned

    //check if the page is already in use
    if (current_pd->virt[pd_entry] == 0 && make) {
        uint32_t phys;
        current_pd->virt[pd_entry] = (uint32_t)kmalloc_ap(0x1000, &phys);
        current_pd->entries[pd_entry] = phys | 0x1;
        if (!is_kernel) {
            current_pd->entries[pd_entry] |= 0x4;
        }
        if (is_writeable) {
            current_pd->entries[pd_entry] |= 0x2;
        }
        current_pd->is_full[pd_entry] = false;
    }
    if ((*(page_table_t *)(current_pd->virt[pd_entry])).pt_entry[pt_entry] & 0x1) {
        kpanic("Attempted to allocate already allocated page!");
    }

    //set the page table entry
    ((page_table_t *)(current_pd->virt[pd_entry]))->pt_entry[pt_entry] = phys | 0x3;

    uint32_t phys_pd_entry = phys >> 22;
    uint32_t phys_pt_entry = (phys >> 12) & 0x3FF;
    if (page_directory_bitmaps[phys_pd_entry] == NULL) {
        page_directory_bitmaps[phys_pd_entry] = kmalloc(sizeof(bitmap_1024_t));
        current_pd->is_full[phys_pd_entry] = false;
    }

    //set the bitmap
    page_directory_bitmaps[phys_pd_entry]->bitmap[phys_pt_entry / 32] |= (1 << (phys_pt_entry % 32));

    //check if the page directory entry is full
    for (uint32_t i = 0; i < 32; i++) {
        if (page_directory_bitmaps[phys_pd_entry]->bitmap[i] != 0xFFFFFFFF) {
            return;
        }
    }
    //the page directory entry is full
    current_pd->is_full[phys_pd_entry] = true;
    return;
}


void heap_expand() {
    //expand the heap by 1MB (1024 pages)
    uint32_t alloc_location = kheap_end;
    page_table_entry_t entry = first_free_page();

    for (uint32_t i = 0; i < 1024; i++) {
        alloc_page(alloc_location + i * 0x1000, (entry.pd_entry * 0x400000) + (entry.pt_entry * 0x1000) + i * 0x1000, true, true, true);
    }

    kheap_end += 0x100000;
    //find the last header, and if free, expand by 1MB
    heap_header_t *header;
    for (header = kheap; header->next != NULL; header = header->next);
    if (header->free) {
        header->length += 0x100000;
    } else {
        //create a new header
        heap_header_t *new_header = (heap_header_t *)kheap_end;
        new_header->magic = HEAP_MAGIC;
        new_header->length = 0x100000 - sizeof(heap_header_t);
        new_header->free = true;
        new_header->next = NULL;
        new_header->prev = header;
        header->next = new_header;
    }
}


void heap_dump()
{
    serial_printf("\n\nHeap dump:\n");
    serial_printf("Address of first header's next field: 0x%x\n", &kheap->next);
    // for each header, print the header, the contents, and the next header
    for (heap_header_t *header = kheap; header != NULL; header = header->next)
    {
        serial_printf("@ 0x%x, length: %x, free: %s\n", header, header->length, header->free ? "true" : "false");
        serial_printf("Contents: 0x%x\n", *(uint32_t *)((uint32_t)header + sizeof(heap_header_t)));
        serial_printf("Next: 0x%x\n", header->next);
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
            uint32_t aligned_addr = (((uint32_t)header + sizeof(heap_header_t)) + 0xFFF) & 0xFFFFF000;
            if (((aligned_addr + size) <= ((uint32_t)header + sizeof(heap_header_t) + header->length)) && header->free)
            {
                // The aligned address block is within the header, and the header is free

                // Check if there is space before the aligned address block for another header
                if (aligned_addr - (uint32_t)header > sizeof(heap_header_t) + sizeof(heap_header_t))
                {
                    uint32_t new_header_address = aligned_addr - sizeof(heap_header_t);

                    uint32_t header_size_old = aligned_addr - sizeof(heap_header_t) - ((uint32_t)header + sizeof(heap_header_t));
                    uint32_t header_size_new = (uint32_t)header + sizeof(heap_header_t) + header->length - aligned_addr;

                    // this allocation needs a header
                    heap_header_t *new_header = (heap_header_t *)(new_header_address);
                    new_header->magic = HEAP_MAGIC;
                    new_header->length = header_size_new;
                    kassert(new_header->length != 0);
                    new_header->next = header->next;
                    new_header->prev = header;
                    new_header->free = false;

                    if (header->next != NULL)
                    {
                        header->next->prev = new_header;
                    }

                    header->next = new_header;
                    header->length = header_size_old;
                    kassert(header->length != 0);
                    header = new_header;

                    kassert(header->length >= size);
                }

                // check if there is space after the aligned address block for another header
                if (((uint32_t)header + sizeof(heap_header_t) + header->length) - (aligned_addr + size) > sizeof(heap_header_t))
                {
                    // there needs to be a header after this allocation
                    uint32_t new_header_address = aligned_addr + size;

                    uint32_t header_size_new = ((uint32_t)header + sizeof(heap_header_t) + header->length) - (aligned_addr + size + sizeof(heap_header_t));
                    uint32_t header_size_old = (aligned_addr + size) - ((uint32_t)header + sizeof(heap_header_t));

                    heap_header_t *new_header = (heap_header_t *)(new_header_address);
                    new_header->magic = HEAP_MAGIC;
                    new_header->length = header_size_new;
                    new_header->next = header->next;
                    new_header->prev = header;
                    new_header->free = true;

                    if (header->next != NULL)
                    {
                        header->next->prev = new_header;
                    }

                    header->next = new_header;
                    header->length = header_size_old;
                }

                header->free = false;
                
                if (phys != NULL)
                {
                    //find the entry via the page table
                    uint32_t pd_entry = aligned_addr >> 22;
                    uint32_t pt_entry = (aligned_addr >> 12) & 0x3FF;
                    *phys = ((page_table_t *)current_pd->virt[pd_entry])->pt_entry[pt_entry] & 0xFFFFF000;
                }
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
                    //find the entry via the page table
                    uint32_t pd_entry = (uint32_t)header >> 22;
                    uint32_t pt_entry = ((uint32_t)header >> 12) & 0x3FF;
                    *phys = (((page_table_t *)current_pd->virt[pd_entry])->pt_entry[pt_entry] & 0xFFFFF000) + ((uint32_t)header & 0xFFF);
                }

                // now we can return the header
                return (void *)((uint32_t)header + sizeof(heap_header_t));
            }
        }
    }

    // We couldn't find a header that was big enough, so we need to expand the heap
    heap_expand();
    if (prealloc_table == NULL) {
        prealloc_table = (page_table_t *)kmalloc_ap(sizeof(page_table_t), &prealloc_phys);
        memset(prealloc_table->pt_entry, 0, sizeof(prealloc_table->pt_entry));
    }
    if (prealloc_bitmap == NULL) {
        prealloc_bitmap = (bitmap_1024_t *)kmalloc(sizeof(bitmap_1024_t));
        memset(prealloc_bitmap->bitmap, 0, sizeof(prealloc_bitmap->bitmap));
    }
    return kmalloc_int(size, align, phys);
}

void kfree_int(void *ptr, bool unaligned)
{
    kassert_msg(kheap != NULL, "kfree called before kheap initialization!");
    heap_header_t *header = (heap_header_t *)((uint32_t)ptr - sizeof(heap_header_t));
    if (!unaligned) {
        kassert_msg(header->magic == HEAP_MAGIC, "Invalid heap header magic number.");
    } else {
        //the header may be up to 0xFFF bytes before the pointer
        while (header->magic != HEAP_MAGIC) {
            header = (heap_header_t *)((uint32_t)header - 1);
        }
        if ((uint32_t)ptr - (uint32_t)header > 0xFFF) {
            kpanic("Header too far away!");
        }
    }
    header->free = true;
    // now we need to merge this header with the next header if it's free
    if (header->next != NULL && header->next->free)
    {
        header->length += header->next->length + sizeof(heap_header_t);
        header->next = header->next->next;
        if (header->next != NULL)
        {
            header->next->prev = header;
        }
    }

    // now we need to merge this header with the previous header if it's free
    if (header->prev != NULL && header->prev->free)
    {
        header->prev->length += header->length + sizeof(heap_header_t);
        header->prev->next = header->next;
        if (header->next != NULL)
        {
            header->next->prev = header->prev;
        }
    }
}

void kfree(void *ptr)
{
    kfree_int(ptr, false);
}

void kfree_a(void *ptr)
{
    kfree_int(ptr, true);
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