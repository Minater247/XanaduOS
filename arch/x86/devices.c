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

device_file_t *dopen(char *path, uint32_t flags) {
    UNUSED(flags);

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
        if (strncmp(current_device->name, path, path_length) == 0) {
            //if we found a device, allocate a device_file_t and return it
            device_file_t *ret = (device_file_t *)kmalloc(sizeof(device_file_t));
            ret->flags = FILE_ISFILE_FLAG | FILE_ISOPEN_FLAG;
            ret->device = current_device;

            return ret;
        }
        current_device = current_device->next;
    }

    //if we didn't find a device, return NULL
    device_file_t *ret = (device_file_t *)kmalloc(sizeof(device_file_t));
    ret->flags = FILE_NOTFOUND_FLAG;
    return ret;
}

device_dir_t *dopendir(char *path, uint32_t flags) {
    UNUSED(flags);

    //the path must be / or any number of slashes, but nothing else!
    while (*path == '/') {
        path++;
    }
    if (*path != '\0') {
        device_dir_t *ret = (device_dir_t *)kmalloc(sizeof(device_dir_t));
        ret->flags = FILE_NOTFOUND_FLAG;
        return ret;
    }

    //allocate a device_dir_t and return it
    device_dir_t *ret = (device_dir_t *)kmalloc(sizeof(device_dir_t));
    ret->flags = FILE_ISOPENDIR_FLAG | FILE_ISDIR_FLAG;
    ret->pos = 0;
    return ret;
}

int dread(void *ptr, uint32_t size, uint32_t nmemb, device_file_t *file) {
    return file->device->read(ptr, size * nmemb);
}

int dwrite(void *ptr, uint32_t size, uint32_t nmemb, device_file_t *file) {
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

int dseek(device_file_t *file, uint32_t offset, uint8_t whence) {
    return file->device->seek(offset, whence);
}

int dtell(device_file_t *file) {
    return file->device->tell();
}

int dclose(device_file_t *file) {
    kfree(file);
    return 0;
}

int dclosedir(device_dir_t *dir) {
    kfree(dir);
    return 0;
}

uint32_t dgetsize(void *fd) {
    UNUSED(fd);
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

void devices_initialize() {
    device_fs.identifier = FILESYSTEM_TYPE_DEVICES;
    device_fs.open = (void*(*)(char*, uint32_t))dopen;
    device_fs.read = (int(*)(char*, uint32_t, uint32_t, void*))dread;
    device_fs.write = (int(*)(char*, uint32_t, uint32_t, void*))dwrite;
    device_fs.seek = (int(*)(void*, uint32_t, uint8_t))dseek;
    device_fs.tell = (int(*)(void*))dtell;
    device_fs.close = (int(*)(void*))dclose;
    device_fs.opendir = (void*(*)(char*, uint32_t))dopendir;
    device_fs.readdir = (simple_return_t(*)(void*))dreaddir;
    device_fs.closedir = (int(*)(void*))dclosedir;
    device_fs.getsize = dgetsize;
    device_fs_registered = register_filesystem(&device_fs);
    if (!device_fs_registered) {
        kpanic("Failed to register device filesystem!\n");
    }
    mount_filesystem(device_fs_registered, "/dev");
}