#include <stdint.h>

#include "../../kernel/include/elf.h"
#include "../../kernel/include/errors.h"
#include "inc_c/string.h"
#include "inc_c/display.h"
#include "inc_c/serial.h"
#include "inc_c/elf_x86.h"
#include "inc_c/memory.h"

elf_load_result_t elf_load_executable(void *elf_file) {
    ELF32_EHDR *elf_header = (ELF32_EHDR *)elf_file;

    //ensure the file is an ELF file
    if (elf_header->e_ident[EI_MAG0] != ELFMAG0 || elf_header->e_ident[EI_MAG1] != ELFMAG1 || elf_header->e_ident[EI_MAG2] != ELFMAG2 || elf_header->e_ident[EI_MAG3] != ELFMAG3) {
        return (elf_load_result_t){-1, NULL}; //not an ELF file
    }

    if (elf_header->e_ident[EI_CLASS] != ELFCLASS32) {
        return (elf_load_result_t){-2, NULL}; //not a 32-bit ELF file
    }

    if (elf_header->e_ident[EI_DATA] != ELFDATA2LSB) {
        return (elf_load_result_t){-3, NULL}; //not a little-endian ELF file
    }

    if (elf_header->e_ident[EI_VERSION] != EV_CURRENT) {
        return (elf_load_result_t){-4, NULL}; //not a current version ELF file
    }

    if (elf_header->e_type != ET_EXEC) {
        return (elf_load_result_t){-5, NULL}; //not an executable ELF file
    }

    serial_printf("Got ELF file\n");
    serial_printf("Header is valid. Entry point: 0x%x\n", elf_header->e_entry);

    //Attempt to load sections
    for (int i = 0; i < elf_header->e_phnum; i++) {
        ELF32_PHDR *program_header = (ELF32_PHDR *)((uint32_t)elf_file + elf_header->e_phoff + (i * elf_header->e_phentsize));

        if (program_header->p_type == PT_NULL) {
            serial_printf("ELF: Ignoring program header %d because it is of type 0\n", i);
        } else if (program_header->p_type == PT_LOAD) {
            //We need to load this section into memory
            serial_printf("ELF: Loading program header %d into memory\n", i);
            serial_printf("\tVirtual address: 0x%x\n", program_header->p_vaddr);
            serial_printf("\tPhysical address: 0x%x\n", program_header->p_paddr);
            serial_printf("\tFile offset: 0x%x\n", program_header->p_offset);
            serial_printf("\tFile size: 0x%x\n", program_header->p_filesz);
            serial_printf("\tMemory size: 0x%x\n", program_header->p_memsz);
            serial_printf("\tFlags: 0x%x\n", program_header->p_flags);
            serial_printf("\tAlignment: 0x%x\n", program_header->p_align);

            kassert(program_header->p_align == 0x1000); //for now we only support x86 page alignment, we'll see if we need to support other alignments later

            //align *down* the virtual address to the nearest page boundary
            uint32_t aligned_vaddr = program_header->p_vaddr & 0xFFFFF000;
            //align *up* the end of the section to the nearest page boundary
            uint32_t aligned_end = (program_header->p_vaddr + program_header->p_memsz + 0xFFF) & 0xFFFFF000;
            //find the number of pages we need to allocate
            uint32_t num_pages = (aligned_end - aligned_vaddr) / 0x1000;
            serial_printf("\n\tVA: 0x%x-0x%x, %d pages\n", aligned_vaddr, aligned_end, num_pages);

            bool is_writable = program_header->p_flags & PF_W;
            for (uint32_t i = 0; i < num_pages; i++) {
                page_table_entry_t first_free = first_free_page();
                alloc_page(aligned_vaddr + (i * 0x1000), (first_free.pd_entry * 0x400000) + (first_free.pt_entry * 0x1000), true, true, is_writable);
            }

            //copy the section into memory
            memcpy((void *)program_header->p_vaddr, (void *)((uint32_t)elf_file + program_header->p_offset), program_header->p_filesz);
            //fill the rest of the section with zeroes
            memset((void *)(program_header->p_vaddr + program_header->p_filesz), 0, program_header->p_memsz - program_header->p_filesz);
        } else {
            serial_printf("ELF: Unhandled program header type %d\n", program_header->p_type);
            return (elf_load_result_t){-6, NULL}; //unhandled program header type
        }
    }


    return (elf_load_result_t){0, (void *)elf_header->e_entry};
}