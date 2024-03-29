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
#define FILE_WRITTEN_FLAG 0x10
#define FILE_MODE_READ 0x20
#define FILE_MODE_WRITE 0x40
#define FILE_IS_TTY 0x80
#define FILE_NOTFOUND_FLAG 0x80000000

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define stdin current_process->fds[0]
#define stdout current_process->fds[1]
#define stderr current_process->fds[2]

typedef struct {
    size_t inode;
    char name[256];
} dirent_t;

#if defined(__ARCH_x86__)

typedef struct {
    uint16_t st_dev; //filesystem ID
    uint16_t pad1;
    uint32_t st_ino; //inode number
    uint16_t st_mode; //file mode
    uint16_t st_nlink; //number of hard links
    uint16_t st_uid; //user ID of owner
    uint16_t st_gid; //group ID of owner
    uint16_t st_rdev; //device ID (if special file)
    uint16_t pad2;
    uint32_t st_size; //total size, in bytes
    uint32_t st_blksize; //blocksize for filesystem I/O
    uint32_t st_blocks; //number of blocks allocated
    uint32_t st_atime; //time of last access
    uint32_t unused1;
    uint32_t st_mtime; //time of last modification
    uint32_t unused2;
    uint32_t st_ctime; //time of last status change
    uint32_t unused3;
    uint32_t unused4;
    uint32_t unused5;
} stat_t;

#endif

typedef struct {
    uint32_t identifier; //SHOULD NOT BE SET TO ANYTHING MEANINGFUL BY CALLER, SET BY REGISTER_FILESYSTEM
    void *(*open)(char *path, char *flags);
    int (*read)(char *buf, size_t size, size_t count, void *file); //all void *file parameters are actually the filesystem's internal representation of a file
    int (*write)(char *buf, size_t size, size_t count, void *file);
    int (*seek)(void *file, size_t offset, int whence);
    size_t (*tell)(void *file);
    int (*close)(void *file);
    void *(*opendir)(char *path);
    int (*stat)(void *file, stat_t *statbuf);
    dirent_t (*readdir)(void *dir);
    int (*closedir)(void *dir);
    void *(*copy)(void *fd); //both should be file_descriptor_t*, but we can't include that here
    dirent_t *(*getdent)(dirent_t *buf, uint32_t entry_num, void *dir);
} filesystem_t;

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

typedef struct fs_node {
    filesystem_t *fs;
    struct fs_node *next;
} fs_node_t;

typedef struct mount_point {
    char *path;
    filesystem_t *fs;
    struct mount_point *next;
} mount_point_t;

void get_path_item(char *path, char *retbuf, uint8_t item);
uint32_t get_path_length(char *path);
int register_filesystem(filesystem_t *to_register);
int mount_filesystem(uint32_t filesystem_id, char *path);
file_descriptor_t *fopen(char *path, char *flags);
int fread(char *buf, size_t size, size_t count, file_descriptor_t *fd);
int fwrite(char *buf, size_t size, size_t count, file_descriptor_t *fd);
int fclose(file_descriptor_t *fd);
dir_descriptor_t *fopendir(char *path);
int fclosedir(dir_descriptor_t *dd);
int fseek(file_descriptor_t *fd, size_t offset, int whence);
size_t ftell(file_descriptor_t *fd);
int fstat(file_descriptor_t *fd, stat_t *statbuf);
file_descriptor_t *copy_descriptor(file_descriptor_t *fd, uint32_t id);
dirent_t *fgetdent(dirent_t *buf, uint32_t entry_num, dir_descriptor_t *fd);

#endif