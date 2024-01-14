#ifndef _arch_elf_H
#define _arch_elf_H

#include <stdint.h>

typedef struct {
    int code;
    void *entry_point;
} elf_load_result_t;

#define ELF_ERR_NONE 0
#define ELF_ERR_NOT_ELF_FILE -1
#define ELF_ERR_NOT_32BIT -2
#define ELF_ERR_NOT_LITTLE_ENDIAN -3
#define ELF_ERR_NOT_CURRENT_VERSION -4
#define ELF_ERR_NOT_EXECUTABLE -5
#define ELF_ERR_INVALID_SECTION -6

elf_load_result_t elf_load_executable(void *elf_file);
elf_load_result_t elf_load_executable_path(char *path);

#endif