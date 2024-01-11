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

typedef struct {
    uint32_t disk_sz;
    uint32_t num_files;
    uint32_t headers_len;
    uint32_t num_root_files;
} __attribute__((packed)) ramdisk_size_t;

typedef struct {
    uint16_t magic;
    uint32_t offset;
    char name[64];
    uint32_t length;
    uint8_t reserved[6];
} __attribute__((packed)) ramdisk_file_header_t;

typedef struct {
    uint16_t magic;
    uint32_t num_files;
    char name[64];
    uint32_t num_blocks;
    uint8_t reserved[6];
} __attribute__((packed)) ramdisk_dir_header_t;

typedef struct {
    uint32_t flags;
    char name[64];
    uint32_t length;
    uint32_t addr;
    uint32_t seek_pos;
} __attribute__((packed)) ramdisk_file_t;

typedef struct {
    uint32_t flags;
    char name[64];
    uint32_t num_files;
    uint32_t idx;
    void *files;
} __attribute__((packed)) ramdisk_dir_t;

void ramdisk_initialize();
ramdisk_file_t *ropen(char *path, uint32_t flags);
int rread(void *ptr, uint32_t size, uint32_t nmemb, ramdisk_file_t *file);
int rseek(ramdisk_file_t *file, uint32_t offset, uint8_t whence);

#endif