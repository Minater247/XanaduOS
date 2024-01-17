#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "../../kernel/include/errors.h"
#include "../../kernel/include/unused.h"
#include "inc_c/string.h"
#include "inc_c/multiboot.h"
#include "../../kernel/include/filesystem.h"
#include "inc_c/memory.h"
#include "inc_c/devices.h"
#include "inc_c/serial.h"

filesystem_t device_fs = {0};
int device_fs_registered = 0;

device_t *device_head = NULL;

device_file_t *dopen(char *path, char *flags) {
    //Skip the leading slashes, if any
    while (*path == '/') {
        path++;
    }

    //If the path is empty, return NULL
    if (*path == '\0') {
        return NULL;
    }

    //Remove the trailing slashes, if any
    uint32_t path_length = strlen(path);
    while (path[path_length - 1] == '/') {
        path_length--;
    }

    //look for a device with the given name using strncmp
    device_t *current_device = device_head;
    while (current_device != NULL) {
        if (strlen(current_device->name) == path_length && strncmp(current_device->name, path, path_length) == 0) {
            //if we found a device, allocate a device_file_t and return it
            device_file_t *ret = (device_file_t *)kmalloc(sizeof(device_file_t));
            ret->flags = FILE_ISFILE_FLAG | FILE_ISOPEN_FLAG;
            ret->device = current_device;

            if (flags[0] == 'r') {
                ret->flags |= FILE_MODE_READ;
            } else if (flags[0] == 'w') {
                ret->flags |= FILE_MODE_WRITE;
            } else if (flags[0] == 'a') {
                ret->flags |= FILE_MODE_WRITE;
            } else {
                ret->flags |= FILE_MODE_READ | FILE_MODE_WRITE;
            }

            if (flags[1] == '+') {
                if (flags[0] == 'r') {
                    ret->flags |= FILE_MODE_WRITE;
                } else if (flags[0] == 'w') {
                    ret->flags |= FILE_MODE_READ;
                } else if (flags[0] == 'a') {
                    ret->flags |= FILE_MODE_READ;
                }
            }

            return ret;
        }
        current_device = current_device->next;
    }

    //if we didn't find a device, return NULL
    device_file_t *ret = (device_file_t *)kmalloc(sizeof(device_file_t));
    ret->flags = FILE_NOTFOUND_FLAG;
    return ret;
}

device_dir_t *dopendir(char *path) {
    UNUSED(path);
    //allocate a device_dir_t and return it
    device_dir_t *ret = (device_dir_t *)kmalloc(sizeof(device_dir_t));
    ret->flags = FILE_ISOPENDIR_FLAG | FILE_ISDIR_FLAG;
    ret->pos = 0;
    return ret;
}

int dread(void *ptr, size_t size, size_t nmemb, device_file_t *file) {
    if (!(file->flags & FILE_ISOPEN_FLAG)) {
        return -1;
    }
    if (!(file->flags & FILE_ISFILE_FLAG)) {
        return -1;
    }
    if (!(file->flags & FILE_MODE_READ)) {
        return -1;
    }
    return file->device->read(ptr, size * nmemb);
}

int dwrite(void *ptr, size_t size, size_t nmemb, device_file_t *file) {
    if (!(file->flags & FILE_ISOPEN_FLAG)) {
        return -1;
    }
    if (!(file->flags & FILE_ISFILE_FLAG)) {
        return -1;
    }
    if (!(file->flags & FILE_MODE_WRITE)) {
        return -1;
    }
    return file->device->write(ptr, size * nmemb);
}

simple_return_t dreaddir(device_dir_t *dir) {
    // Read the nth device, or if we don't have that many devices, return NULL
    device_t *current_device = device_head;
    for (uint32_t i = 0; i < dir->pos; i++) {
        if (current_device == NULL) {
            return (simple_return_t){FILE_NOTFOUND_FLAG, NULL};
        }
        current_device = current_device->next;
    }

    return (simple_return_t){0, current_device->name};
}

int dseek(device_file_t *file, size_t offset, int whence) {
    if (!(file->flags & FILE_ISOPEN_FLAG)) {
        return -1;
    }
    if (!(file->flags & FILE_ISFILE_FLAG)) {
        return -1;
    }
    return file->device->seek(offset, whence);
}

size_t dtell(device_file_t *file) {
    if (!(file->flags & FILE_ISOPEN_FLAG)) {
        return -1;
    }
    if (!(file->flags & FILE_ISFILE_FLAG)) {
        return -1;
    }
    return file->device->tell();
}

int dclose(device_file_t *file) {
    if (!(file->flags & FILE_ISOPEN_FLAG)) {
        return -1;
    }
    kfree(file);
    return 0;
}

int dclosedir(device_dir_t *dir) {
    if (!(dir->flags & FILE_ISOPENDIR_FLAG)) {
        return -1;
    }
    kfree(dir);
    return 0;
}

int device_rw_empty(void *ptr, size_t size) {
    UNUSED(ptr);
    UNUSED(size);
    return 0;
}

int device_seek_empty(size_t offset, int whence) {
    UNUSED(offset);
    UNUSED(whence);
    return 0;
}

size_t device_tell_empty() {
    return 0;
}

int register_device(device_t *device) {
    if (device_head == NULL) {
        device_head = device;
        return 1;
    }

    device_t *current_device = device_head;
    while (current_device->next != NULL) {
        current_device = current_device->next;
    }
    current_device->next = device;

    return 1;
}

int unregister_device(device_t *device) {
    if (device_head == NULL) {
        return 0;
    }

    if (device_head == device) {
        device_head = device->next;
        return 1;
    }

    device_t *current_device = device_head;
    while (current_device->next != NULL) {
        if (current_device->next == device) {
            current_device->next = device->next;
            return 1;
        }
        current_device = current_device->next;
    }
    return 0;
}

void *dcopy(void *fd) {
    device_file_t *file = (device_file_t*)fd;
    device_file_t *ret = (device_file_t*)kmalloc(sizeof(device_file_t));
    ret->flags = file->flags;
    ret->device = file->device;
    return ret;
}

int dstat(void *file_in, stat_t *statbuf) {
    device_file_t *file = (device_file_t*)file_in; //for some reason, this is necessary to get the compiler to stop complaining
    if (!(file->flags & FILE_ISOPEN_FLAG)) {
        return -1;
    }

    statbuf->st_dev = device_fs.identifier;
    statbuf->pad1 = 0;
    statbuf->st_ino = 0;
    statbuf->st_mode = file->flags & (FILE_MODE_READ | FILE_MODE_WRITE);
    statbuf->st_nlink = 0;
    statbuf->st_uid = 0;
    statbuf->st_gid = 0;
    statbuf->st_rdev = 0;
    statbuf->pad2 = 0;
    statbuf->st_size = 0;
    statbuf->st_blksize = 0;
    statbuf->st_blocks = 0;
    statbuf->st_atime = 0;
    statbuf->unused1 = 0;
    statbuf->st_mtime = 0;
    statbuf->unused2 = 0;
    statbuf->st_ctime = 0;
    statbuf->unused3 = 0;
    statbuf->unused4 = 0;
    statbuf->unused5 = 0;

    return 0;
}

void devices_initialize() {
    device_fs.identifier = FILESYSTEM_TYPE_DEVICES;
    device_fs.open = (void*(*)(char*, char*))dopen;
    device_fs.read = (int(*)(char*, size_t, size_t, void*))dread;
    device_fs.write = (int(*)(char*, size_t, size_t, void*))dwrite;
    device_fs.seek = (int(*)(void*, size_t, int))dseek;
    device_fs.tell = (size_t(*)(void*))dtell;
    device_fs.close = (int(*)(void*))dclose;
    device_fs.opendir = (void*(*)(char*))dopendir;
    device_fs.readdir = (simple_return_t(*)(void*))dreaddir;
    device_fs.closedir = (int(*)(void*))dclosedir;
    device_fs.copy = dcopy;
    device_fs_registered = register_filesystem(&device_fs);
    if (!device_fs_registered) {
        kpanic("Failed to register device filesystem!\n");
    }
    mount_filesystem(device_fs_registered, "/dev");
}