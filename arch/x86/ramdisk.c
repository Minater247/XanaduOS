#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "../../kernel/include/errors.h"
#include "../../kernel/include/unused.h"
#include "inc_c/string.h"
#include "inc_c/multiboot.h"
#include "../../kernel/include/filesystem.h"
#include "inc_c/memory.h"
#include "inc_c/ramdisk.h"
#include "inc_c/serial.h"

ramdisk_size_t *ramdisk;

filesystem_t ramdisk_fs = {0};
int ramdisk_fs_registered = 0;

ramdisk_file_t *ropen(char *path, uint32_t flags) {
    UNUSED(flags);

    uint8_t item = 0;
    char path_item[64];
    uint32_t path_length = get_path_length(path) - 1;
    get_path_item(path, path_item, item);

    uint32_t offset = sizeof(ramdisk_size_t);
    
    while (offset < ramdisk->headers_len) {
        uint16_t magic = *(uint16_t*)((uint32_t)ramdisk + offset);
        if (magic == DIR_ENTRY_MAGIC) {
            ramdisk_dir_header_t *dir_header = (ramdisk_dir_header_t*)((uint32_t)ramdisk + offset);
            if (strncmp(dir_header->name, path_item, 64) == 0) {
                //found this level of the path
                if (item == path_length) {
                    //That's not right, we're looking for a file
                    ramdisk_file_t *file = (ramdisk_file_t*)kmalloc(sizeof(ramdisk_file_t));
                    file->flags = FILE_NOTFOUND_FLAG | FILE_ISDIR_FLAG;
                    return file;
                } else {
                    //This is one of the directories we're looking for, so keep going
                    item++;
                    get_path_item(path, path_item, item);
                    offset += sizeof(ramdisk_dir_header_t);
                    continue;
                }
            }
            offset += sizeof(ramdisk_dir_header_t) * dir_header->num_blocks;
        } else if (magic == FILE_ENTRY_MAGIC) {
            ramdisk_file_header_t *file_header = (ramdisk_file_header_t*)((uint32_t)ramdisk + offset);
            if (strncmp(file_header->name, path_item, 64) == 0) {
                //found this level of the path
                if (item == path_length) {
                    //we're at the end of the path, so return the file
                    ramdisk_file_t *file = (ramdisk_file_t*)kmalloc(sizeof(ramdisk_file_t));
                    file->flags = FILE_ISOPEN_FLAG | FILE_ISFILE_FLAG;
                    for (uint8_t i = 0; i < 64; i++) {
                        file->name[i] = file_header->name[i];
                    }
                    file->length = file_header->length;
                    file->addr = (uint32_t)ramdisk + ramdisk->headers_len + file_header->offset;
                    file->seek_pos = 0;
                    return file;
                } else {
                    //we're not at the end of the path, so return an error
                    ramdisk_file_t *file = (ramdisk_file_t*)kmalloc(sizeof(ramdisk_file_t));
                    file->flags = FILE_NOTFOUND_FLAG;
                    return file;
                }
            }
            offset += sizeof(ramdisk_file_header_t);
        } else {
            //wait, what?
            kpanic("Invalid magic number in ramdisk: %x\n", magic);
        }
    }

    //if we get here, we didn't find the file
    ramdisk_file_t *file = (ramdisk_file_t*)kmalloc(sizeof(ramdisk_file_t));
    file->flags = FILE_NOTFOUND_FLAG;
    return file;
}

ramdisk_dir_t *ropendir(char *path, uint32_t flags) {
    UNUSED(flags);

    uint8_t item = 0;
    char path_item[64];
    uint32_t path_length = get_path_length(path) - 1;
    get_path_item(path, path_item, item);

    //if the path is just "/", return the root directory
    if (strncmp(path, "/", 2) == 0) {
        ramdisk_dir_t *dir = (ramdisk_dir_t*)kmalloc(sizeof(ramdisk_dir_t));
        dir->flags = FILE_ISOPENDIR_FLAG | FILE_ISDIR_FLAG;
        dir->name[0] = '/';
        dir->name[1] = '\0';
        dir->num_files = ramdisk->num_root_files;
        dir->idx = 0;
        dir->files = (void*)((uint32_t)ramdisk + sizeof(ramdisk_size_t));
        return dir;
    }

    uint32_t offset = sizeof(ramdisk_size_t);
    
    while (offset < ramdisk->headers_len) {
        uint16_t magic = *(uint16_t*)((uint32_t)ramdisk + offset);
        if (magic == DIR_ENTRY_MAGIC) {
            ramdisk_dir_header_t *dir_header = (ramdisk_dir_header_t*)((uint32_t)ramdisk + offset);
            if (strncmp(dir_header->name, path_item, 64) == 0) {
                //found this level of the path
                if (item == path_length) {
                    //we're at the end of the path, so return the directory
                    ramdisk_dir_t *dir = (ramdisk_dir_t*)kmalloc(sizeof(ramdisk_dir_t));
                    dir->flags = FILE_ISOPENDIR_FLAG | FILE_ISDIR_FLAG;
                    for (uint8_t i = 0; i < 64; i++) {
                        dir->name[i] = dir_header->name[i];
                    }
                    dir->num_files = dir_header->num_files;
                    dir->idx = 0;
                    dir->files = (void*)((uint32_t)ramdisk + offset + sizeof(ramdisk_dir_header_t));
                    return dir;
                } else {
                    //This is one of the directories we're looking for, so keep going
                    item++;
                    get_path_item(path, path_item, item);
                    offset += sizeof(ramdisk_dir_header_t);
                    continue;
                }
            }
            offset += sizeof(ramdisk_dir_header_t) * dir_header->num_blocks;
        } else if (magic == FILE_ENTRY_MAGIC) {
            ramdisk_file_header_t *file_header = (ramdisk_file_header_t*)((uint32_t)ramdisk + offset);
            if (strncmp(file_header->name, path_item, 64) == 0) {
                //found this level of the path
                if (item == path_length) {
                    //that's not right, we're looking for a directory
                    ramdisk_dir_t *dir = (ramdisk_dir_t*)kmalloc(sizeof(ramdisk_dir_t));
                    dir->flags = FILE_NOTFOUND_FLAG | FILE_ISDIR_FLAG;
                    return dir;
                } else {
                    //we're not at the end of the path, so return an error
                    ramdisk_dir_t *dir = (ramdisk_dir_t*)kmalloc(sizeof(ramdisk_dir_t));
                    dir->flags = FILE_NOTFOUND_FLAG;
                    return dir;
                }
            }
            offset += sizeof(ramdisk_file_header_t);
        } else {
            //wait, what?
            kpanic("Invalid magic number in ramdisk: %x\n", magic);
        }
    }

    //if we get here, we didn't find the file
    ramdisk_dir_t *dir = (ramdisk_dir_t*)kmalloc(sizeof(ramdisk_dir_t));
    dir->flags = FILE_NOTFOUND_FLAG;
    return dir;
}

int rread(void *ptr, uint32_t size, uint32_t nmemb, ramdisk_file_t *file) {
    if (!(file->flags & FILE_ISOPEN_FLAG)) {
        return -1;
    };
    if (file->seek_pos >= file->length) {
        return 0;
    }
    uint32_t bytes_to_read = size * nmemb;
    if (file->seek_pos + bytes_to_read > file->length) {
        bytes_to_read = file->length - file->seek_pos;
    }
    for (uint32_t i = 0; i < bytes_to_read; i++) {
        ((char*)ptr)[i] = *(char*)(file->addr + file->seek_pos + i);
    }
    file->seek_pos += bytes_to_read;
    return bytes_to_read;
}

simple_return_t rreaddir(ramdisk_dir_t *dir) {

    if (!(dir->flags & FILE_ISOPENDIR_FLAG)) {
        simple_return_t ret = {0, NULL};
        ret.flags |= FILE_NOTFOUND_FLAG;
        return ret;
    }

    if (dir->idx >= dir->num_files) {
        simple_return_t ret = {0, NULL};
        ret.flags |= FILE_NOTFOUND_FLAG;
        return ret;
    }

    ramdisk_file_header_t *file_header = (ramdisk_file_header_t*)((uint32_t)dir->files);
    for (uint32_t i = 0; i < dir->idx; i++) {
        //if this is a directory, skip over it and all its blocks.
        if (file_header->magic == DIR_ENTRY_MAGIC) {
            file_header = (ramdisk_file_header_t*)((uint32_t)file_header + sizeof(ramdisk_dir_header_t) * (((ramdisk_dir_header_t*)file_header)->num_blocks + 1));
        } else {
            file_header = (ramdisk_file_header_t*)((uint32_t)file_header + sizeof(ramdisk_file_header_t));
        }
    }

    simple_return_t ret = {0, file_header->name};
    ret.flags |= FILE_ISFILE_FLAG;
    dir->idx++;
    return ret;
}

int rseek(ramdisk_file_t *file, uint32_t offset, uint8_t whence) {
    if (!(file->flags & FILE_ISOPEN_FLAG)) {
        return -1;
    }

    switch (whence) {
        case SEEK_SET:
            file->seek_pos = offset;
            break;
        case SEEK_CUR:
            file->seek_pos += offset;
            break;
        case SEEK_END:
            file->seek_pos = file->length + offset;
            break;
        default:
            return -1;
    }

    if (file->seek_pos > file->length) {
        file->seek_pos = file->length;
    }

    return 0;
}

int rtell(ramdisk_file_t *file) {
    if (!(file->flags & FILE_ISOPEN_FLAG)) {
        return -1;
    }

    return file->seek_pos;
}

int rclose(ramdisk_file_t *file) {
    if (!(file->flags & FILE_ISOPEN_FLAG)) {
        return -1;
    }

    file->flags &= ~FILE_ISOPEN_FLAG;
    kfree(file);
    return 0;
}

int rclosedir(ramdisk_dir_t *dir) {
    if (!(dir->flags & FILE_ISOPEN_FLAG)) {
        return -1;
    }

    dir->flags &= ~FILE_ISOPEN_FLAG;
    kfree(dir);
    return 0;
}

uint32_t getsize(void *fd) {
    //depending on whether this is a file or directory, return the appropriate size
    if (*(uint32_t*)fd & FILE_ISFILE_FLAG) {
        return ((ramdisk_file_t*)fd)->length;
    } else if (*(uint32_t*)fd & FILE_ISDIR_FLAG) {
        return ((ramdisk_dir_t*)fd)->num_files;
    } else {
        return 0;
    }
}

void ramdisk_initialize(multiboot_info_t *mboot_info) {
    uint32_t ramdisk_addr_multiboot = 0;
    if (mboot_info->flags & MULTIBOOT_INFO_MODS) {
        //set the ramdisk address to the address of the first module with the string "RAMDISK"
        multiboot_module_t *module = (multiboot_module_t*)((uint32_t)mboot_info->mods_addr + 0xC0000000);
        while (module->mod_end != 0) {
            if (strncmp((char*)((uint32_t)module->cmdline + 0xC0000000), "RAMDISK", 7) == 0) {
                ramdisk_addr_multiboot = module->mod_start;
                break;
            }
            module++;
        }
    } else {
        kpanic("No ramdisk found!\n");
    }

    kassert(ramdisk_addr_multiboot != 0);

    ramdisk = (ramdisk_size_t*)(ramdisk_addr_multiboot + 0xC0000000);

    ramdisk_fs.identifier = FILESYSTEM_TYPE_RAMDISK;
    ramdisk_fs.open = (void*(*)(char*, uint32_t))ropen;
    ramdisk_fs.read = (int(*)(char*, uint32_t, uint32_t, void*))rread;
    ramdisk_fs.write = NULL;
    ramdisk_fs.seek = (int(*)(void*, uint32_t, uint8_t))rseek;
    ramdisk_fs.tell = (int(*)(void*))rtell;
    ramdisk_fs.close = (int(*)(void*))rclose;
    ramdisk_fs.opendir = (void*(*)(char*, uint32_t))ropendir;
    ramdisk_fs.readdir = (simple_return_t(*)(void*))rreaddir;
    ramdisk_fs.closedir = (int(*)(void*))rclosedir;
    ramdisk_fs.getsize = getsize;
    ramdisk_fs_registered = register_filesystem(&ramdisk_fs);
    if (!ramdisk_fs_registered) {
        kpanic("Failed to register ramdisk filesystem!\n");
    }
    mount_filesystem(ramdisk_fs_registered, "/mnt/ramdisk");
}