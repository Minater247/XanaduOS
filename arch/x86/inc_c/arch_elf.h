#ifndef _arch_elf_H
#define _arch_elf_H

#include <stdint.h>

typedef struct {
    int code;
    void *entry_point;
} elf_load_result_t;

elf_load_result_t elf_load_executable(void *elf_file);

#endif