#ifndef _FILESYSTEM_H
#define _FILESYSTEM_H

#include <stdint.h>
#include "inc_c/ramdisk.h"

#define FILESYSTEM_TYPE_RAMDISK 0x1
#define FILESYSTEM_TYPE_DEVICES 0x2

#define FILE_ISOPEN_FLAG 0x1
#define FILE_ISOPENDIR_FLAG 0x2
#define FILE_ISDIR_FLAG 0x4 //set if found dir, expecting file
#define FILE_ISFILE_FLAG 0x8 //set if found file, expecting dir
#define FILE_NOTFOUND_FLAG 0x80000000

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

typedef struct {
    uint32_t flags;
    char *name;
} simple_return_t;

typedef struct {
    uint32_t identifier; //SHOULD NOT BE SET TO ANYTHING MEANINGFUL BY CALLER, SET BY REGISTER_FILESYSTEM
    void *(*open)(char *path, uint32_t flags);
    int (*read)(char *buf, uint32_t size, uint32_t count, void *file); //all void *file parameters are actually the filesystem's internal representation of a file
    int (*write)(char *buf, uint32_t size, uint32_t count, void *file);
    int (*seek)(void *file, uint32_t offset, uint8_t whence);
    int (*tell)(void *file);
    int (*close)(void *file);
    void *(*opendir)(char *path, uint32_t flags);
    uint32_t (*getsize)(void *file);
    simple_return_t (*readdir)(void *dir);
    int (*closedir)(void *dir);
} filesystem_t;

typedef struct fs_node {
    filesystem_t *fs;
    struct fs_node *next;
} fs_node_t;

typedef struct mount_point {
    char *path;
    filesystem_t *fs;
    struct mount_point *next;
} mount_point_t;

//file descriptor
//The first item in fs_data must be a uint32_t containing the flags for the file.
typedef struct {
    uint32_t id;
    uint32_t flags;
    filesystem_t *fs;
    void *fs_data;
} file_descriptor_t;

typedef struct {
    uint32_t id;
    uint32_t flags;
    filesystem_t *fs;
    void *fs_data;
} dir_descriptor_t;

void get_path_item(char *path, char *retbuf, uint8_t item);
uint32_t get_path_length(char *path);
int register_filesystem(filesystem_t *to_register);
int mount_filesystem(uint32_t filesystem_id, char *path);
file_descriptor_t fopen(char *path, uint32_t flags);
int fread(char *buf, uint32_t size, uint32_t count, file_descriptor_t *fd);
int fwrite(char *buf, uint32_t size, uint32_t count, file_descriptor_t *fd);
int fclose(file_descriptor_t *fd);
dir_descriptor_t fopendir(char *path, uint32_t flags);
simple_return_t freaddir(dir_descriptor_t *dd);
int fclosedir(dir_descriptor_t *dd);
uint32_t fgetsize(void *fd);
int fseek(file_descriptor_t *fd, uint32_t offset, uint8_t whence);
int ftell(file_descriptor_t *fd);

#endif