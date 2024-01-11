#ifndef _ELF_x86_H
#define _ELF_x86_H

#include <stdint.h>

typedef struct {
    int code;
    void *entry_point;
} elf_load_result_t;

elf_load_result_t elf_load_executable(void *elf_file);

#endif