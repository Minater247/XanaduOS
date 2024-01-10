#ifndef _RAMDISK_H
#define _RAMDISK_H

//The format of a ramdisk is as follows:
//- The first 4 bytes are the number of files in the ramdisk (uint64)
//- The next 4 bytes are the length of the file list (uint64)
//- A list of file entries or directory entries

//A file entry is as follows:
//  - 2 bytes: a magic number declaring this is a file entry (uint16)
//  - 4 bytes: the offset of the file in the ramdisk (uint64)
//  - 64 bytes: the name of the file (null terminated)
//  - 4 bytes: the length of the file (uint64)

//A directory entry is as follows:
//  - 2 bytes: a magic number declaring this is a directory entry (uint16)
//  - 64 bytes: the name of the directory (null terminated)
//  - 4 bytes: the number of files in the directory (uint64)
//  - The file and directory entries for the directory
//  - 2 bytes: a magic number declaring the end of the directory (uint16)

#include <stdint.h>

#define FILE_ENTRY_MAGIC 0xBAE7
#define DIR_ENTRY_MAGIC 0x7EAB
#define DIR_END_MAGIC 0x7EAC

//structs should be packed with __attribute__((packed)) to avoid padding
typedef struct {
    uint32_t num_files;
    uint32_t file_list_length;
} __attribute__((packed)) ramdisk_header_t;

typedef struct {
    uint16_t magic;
    uint32_t offset;
    char name[64];
    uint32_t length;
} __attribute__((packed)) ramdisk_file_entry_t; //internal representation of a file

typedef struct {
    uint32_t flags;
    char name[64];
    uint32_t length;
    uint32_t addr;
    uint32_t seek_pos;
} __attribute__((packed)) ramdisk_file_t; //external representation of a file

typedef struct {
    uint32_t flags;
    char name[64];
    uint32_t num_files;
    uint32_t idx;
    void *files;
} __attribute__((packed)) ramdisk_dir_t; //external representation of a directory
//Directories have no internal representation due to their variable data contents length.

//dir entry isn't really like something that can be represented with a struct,
//so we don't have one for it

void ramdisk_initialize();
void ramdisk_test();
ramdisk_file_t *ropen(char *path, uint32_t flags);
int rread(void *ptr, uint32_t size, uint32_t nmemb, ramdisk_file_t *file);
int rseek(ramdisk_file_t *file, uint32_t offset, uint8_t whence);

#endif