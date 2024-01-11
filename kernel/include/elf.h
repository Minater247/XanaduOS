#ifndef _ELF_H
#define _ELF_H

#include <stdint.h>

#if defined(__ARCH_x86__)


// --------------- General ---------------
typedef uint32_t ELF32_ADDR;
typedef uint16_t ELF32_HALF;
typedef uint32_t ELF32_OFF;
typedef int32_t  ELF32_SWORD;
typedef uint32_t ELF32_WORD;

#define EI_NIDENT 16

typedef struct {
    unsigned char   e_ident[EI_NIDENT];
    ELF32_HALF      e_type;
    ELF32_HALF      e_machine;
    ELF32_WORD      e_version;
    ELF32_ADDR      e_entry;
    ELF32_OFF       e_phoff;
    ELF32_OFF       e_shoff;
    ELF32_WORD      e_flags;
    ELF32_HALF      e_ehsize;
    ELF32_HALF      e_phentsize;
    ELF32_HALF      e_phnum;
    ELF32_HALF      e_shentsize;
    ELF32_HALF      e_shnum;
    ELF32_HALF      e_shstrndx;
} ELF32_EHDR;

#define ET_NONE     0
#define ET_REL      1
#define ET_EXEC     2
#define ET_DYN      3
#define ET_CORE     4
#define ET_LOPROC   0xff00
#define ET_HIPROC   0xffff

#define EM_NONE     0
#define EM_M32      1
#define EM_SPARC    2
#define EM_386      3
#define EM_68K      4
#define EM_88K      5
#define EM_860      7
#define EM_MIPS     8

#define EV_NONE     0
#define EV_CURRENT  1

#define EI_MAG0     0
#define EI_MAG1     1
#define EI_MAG2     2
#define EI_MAG3     3
#define EI_CLASS    4
#define EI_DATA     5
#define EI_VERSION  6
#define EI_PAD      7
#define EI_NIDENT   16

#define ELFMAG0     0x7F
#define ELFMAG1     'E'
#define ELFMAG2     'L'
#define ELFMAG3     'F'

#define ELFCLASSNONE    0
#define ELFCLASS32      1
#define ELFCLASS64      2

#define ELFDATANONE 0
#define ELFDATA2LSB 1
#define ELFDATA2MSB 2

#define SHN_UNDEF       0
#define SHN_LORESERVE   0xFF00
#define SHN_LOPROC      0xFF00
#define SHN_HIPROC      0xFF1F
#define SHN_ABS         0xFFF1
#define SHN_COMMON      0xFFF2
#define SHN_HIRESERVE   0xFFFF

typedef struct {
    ELF32_WORD      sh_name;
    ELF32_WORD      sh_type;
    ELF32_WORD      sh_flags;
    ELF32_ADDR      sh_addr;
    ELF32_OFF       sh_offset;
    ELF32_WORD      sh_size;
    ELF32_WORD      sh_link;
    ELF32_WORD      sh_info;
    ELF32_WORD      sh_addralign;
    ELF32_WORD      sh_entsize;
} ELF32_SHDR;

#define SHT_NULL        0
#define SHT_PROGBITS    1
#define SHT_SYMTAB      2
#define SHT_STRTAB      3
#define SHT_RELA        4
#define SHT_HASH        5
#define SHT_DYNAMIC     6
#define SHT_NOTE        7
#define SHT_NOBITS      8
#define SHT_REL         9
#define SHT_SHLIB       10
#define SHT_DYNSYM      11
#define SHT_LOPROC      0x70000000
#define SHT_HIPROC      0x7FFFFFFF
#define SHT_LOUSER      0x80000000
#define SHT_HIUSER      0xFFFFFFFF

#define SHF_WRITE       0x1
#define SHF_ALLOC       0x2
#define SHF_EXECINSTR   0x4
#define SHF_MASKPROC    0xF0000000

#define STN_UNDEF       0

typedef struct {
    ELF32_WORD      st_name;
    ELF32_ADDR      st_value;
    ELF32_WORD      st_size;
    unsigned char   st_info;
    unsigned char   st_other;
    ELF32_HALF      st_shndx;
} ELF32_SYM;

#define ELF32_ST_BIND(i)    ((i)>>4)
#define ELF32_ST_TYPE(i)    ((i)&0xf)
#define ELF32_ST_INFO(b,t)  (((b)<<4)+((t)&0xf))

#define STB_LOCAL       0
#define STB_GLOBAL      1
#define STB_WEAK        2
#define STB_LOPROC      13
#define STB_HIPROC      15

#define STT_NOTYPE      0
#define STT_OBJECT      1
#define STT_FUNC        2
#define STT_SECTION     3
#define STT_FILE        4
#define STT_LOPROC      13
#define STT_HIPROC      15

typedef struct {
    ELF32_ADDR      r_offset;
    ELF32_WORD      r_info;
} ELF32_REL;

typedef struct {
    ELF32_ADDR      r_offset;
    ELF32_WORD      r_info;
    ELF32_SWORD     r_addend;
} ELF32_RELA;

#define ELF32_R_SYM(i)      ((i)>>8)
#define ELF32_R_TYPE(i)     ((unsigned char)(i))
#define ELF32_R_INFO(s,t)   (((s)<<8)+(unsigned char)(t))

#define R_386_NONE      0
#define R_386_32        1
#define R_386_PC32      2
#define R_386_GOT32     3
#define R_386_PLT32     4
#define R_386_COPY      5
#define R_386_GLOB_DAT  6
#define R_386_JMP_SLOT  7
#define R_386_RELATIVE  8
#define R_386_GOTOFF    9
#define R_386_GOTPC     10



// --------------- Loading --------------- 
typedef struct {
    ELF32_WORD      p_type;
    ELF32_OFF       p_offset;
    ELF32_ADDR      p_vaddr;
    ELF32_ADDR      p_paddr;
    ELF32_WORD      p_filesz;
    ELF32_WORD      p_memsz;
    ELF32_WORD      p_flags;
    ELF32_WORD      p_align;
} ELF32_PHDR;

#define PT_NULL         0
#define PT_LOAD         1
#define PT_DYNAMIC      2
#define PT_INTERP       3
#define PT_NOTE         4
#define PT_SHLIB        5
#define PT_PHDR         6
#define PT_LOPROC       0x70000000
#define PT_HIPROC       0x7FFFFFFF

#define PF_X            0x1
#define PF_W            0x2
#define PF_R            0x4
#define PF_MASKOS       0x0ff00000
#define PF_MASKPROC     0xf0000000

#define DT_NULL         0
#define DT_NEEDED       1
#define DT_PLTRELSZ     2
#define DT_PLTGOT       3
#define DT_HASH         4
#define DT_STRTAB       5
#define DT_SYMTAB       6
#define DT_RELA         7
#define DT_RELASZ       8
#define DT_RELAENT      9
#define DT_STRSZ        10
#define DT_SYMENT       11
#define DT_INIT         12
#define DT_FINI         13
#define DT_SONAME       14
#define DT_RPATH        15
#define DT_SYMBOLIC     16

#endif

#endif