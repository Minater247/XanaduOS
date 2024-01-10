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

ramdisk_header_t *ramdisk;

filesystem_t ramdisk_fs = {0};
int ramdisk_fs_registered = 0;

//The format of a ramdisk is as follows:
//- The first 4 bytes are the number of files in the ramdisk (uint32)
//- The next 4 bytes are the length of the file list (uint32)
//- A list of file entries or directory entries

//A file entry is as follows:
//  - 2 bytes: a magic number declaring this is a file entry (uint16)
//  - 4 bytes: the offset of the file in the ramdisk (uint32)
//  - 64 bytes: the name of the file (null terminated)
//  - 4 bytes: the length of the file (uint32)

//A directory entry is as follows:
//  - 2 bytes: a magic number declaring this is a directory entry (uint16)
//  - 64 bytes: the name of the directory (null terminated)
//  - 4 bytes: the number of files in the directory (uint32)
//  - The file and directory entries for the directory
//  - 2 bytes: a magic number declaring the end of the directory (uint16)


//takes a path and a pointer to an empty file_t, and returns the file info
//to the file_t
ramdisk_file_t *ropen(char *path, uint32_t flags) {
    UNUSED(flags); //ramdisk presently has no use for flags, as it is read-only
    char buf[64] = {0};

    //if the path starts with a /, then move past it
    if (path[0] == '/') {
        path++;
    }

    //get the first item in the path
    uint32_t item = 0;
    uint32_t length = get_path_length(path);
    get_path_item(path, buf, item);

    uint32_t offset = sizeof(ramdisk_header_t);
    uint32_t file_list_length = ramdisk->file_list_length;

    serial_printf("Opening file %s\n", path);

    while (offset < file_list_length) {
        uint16_t magic = *(uint16_t*)((uint32_t)ramdisk + offset);
        serial_printf("Magic number: %x\n", magic);
        if (magic == FILE_ENTRY_MAGIC) {
            //if this is the last item in the path, then we found the file
            ramdisk_file_entry_t *file = (ramdisk_file_entry_t*)((uint32_t)ramdisk + offset);
            if (item == length && strcmp(buf, file->name) == 0) {
                ramdisk_file_t *ret = (ramdisk_file_t*)kmalloc(sizeof(ramdisk_file_t));
                ret->length = file->length;
                ret->addr = (uint32_t)ramdisk + file_list_length + file->offset + 16;
                ret->seek_pos = 0;
                uint32_t i = 0;
                while (file->name[i] != '\0') {
                    ret->name[i] = file->name[i];
                    i++;
                }
                ret->name[i] = '\0';
                ret->flags = FILE_ISOPEN_FLAG;
                return ret;
            } else {
                offset += sizeof(ramdisk_file_entry_t);
            }
            //otherwise, we need to go deeper
        } else if (magic == DIR_ENTRY_MAGIC) {
            //next 64 bytes are the name of the directory
            offset += 2;
            if (strcmp(buf, (char*)((uint32_t)ramdisk + offset)) == 0) {
                //we found the directory
                if (item == length) {
                    //...tried to open directory as a file
                    ramdisk_file_t *ret = (ramdisk_file_t*)kmalloc(sizeof(ramdisk_file_t));
                    ret->flags = FILE_NOTFOUND_FLAG | FILE_ISDIR_FLAG;
                    return ret;
                }
                item++;
                offset += 64;
                offset += 4;
                get_path_item(path, buf, item);
            } else {
                //we didn't find the directory, so skip it
                serial_printf("Skipping directory %s\n", (char*)((uint32_t)ramdisk + offset));
                serial_printf("Contents:\n");
                for (uint8_t i = 0; i < 64; i++) {
                    serial_printf("%c", ((char*)((uint32_t)ramdisk + offset))[i]);
                }
                serial_printf("\n0x");
                for (uint8_t i = 0; i < 8; i++) {
                    serial_printf("%x ", ((uint8_t*)((uint32_t)ramdisk + offset + 64))[i]);
                }
                serial_printf("\n");

                offset += 64;
                offset += 4;

                serial_printf("Num after: %x\n", *(uint16_t*)((uint32_t)ramdisk + offset));

                //loop until we find the end of the directory
                uint32_t depth = 1;
                while (depth > 0) {
                    if (*(uint16_t*)((uint32_t)ramdisk + offset) == DIR_ENTRY_MAGIC) {
                        serial_printf("Got DIR_ENTRY_MAGIC\n");
                        depth++;
                        offset += 2;
                        offset += 64;
                        offset += 4;
                    } else if (*(uint16_t*)((uint32_t)ramdisk + offset) == DIR_END_MAGIC) {
                        serial_printf("Got DIR_END_MAGIC\n");
                        depth--;
                        offset += 2;
                    } else if (*(uint16_t*)((uint32_t)ramdisk + offset) == FILE_ENTRY_MAGIC) {
                        serial_printf("Got FILE_ENTRY_MAGIC\n");
                        offset += sizeof(ramdisk_file_entry_t);
                    } else {
                        serial_printf("Invalid magic number: %x @ offset 0x%x\n", *(uint16_t*)((uint32_t)ramdisk + offset), offset);
                        kpanic("Invalid magic number: %x @ offset 0x%x\n", *(uint16_t*)((uint32_t)ramdisk + offset), offset);
                    }
                }
            }
        } else if (magic == DIR_END_MAGIC) {
            //we found the end of the directory, so we didn't find the file
            ramdisk_file_t *ret = (ramdisk_file_t*)kmalloc(sizeof(ramdisk_file_t));
            ret->flags = FILE_NOTFOUND_FLAG;
            return ret;
        } else {
            kpanic("Invalid magic number: %x\n", magic);
        }
    }

    ramdisk_file_t *ret = (ramdisk_file_t*)kmalloc(sizeof(ramdisk_file_t));
    ret->flags = FILE_NOTFOUND_FLAG;
    return ret;
}

ramdisk_dir_t *ropendir(char *path, uint32_t flags) {
    UNUSED(flags); //ramdisk presently has no use for flags, as it is read-only
    char buf[64] = {0};

    //if the path starts with a /, then move past it
    if (path[0] == '/') {
        path++;
    }

    //get the first item in the path
    uint32_t item = 0;
    uint32_t length = get_path_length(path);
    get_path_item(path, buf, item);

    uint32_t offset = sizeof(ramdisk_header_t);
    uint32_t file_list_length = ramdisk->file_list_length;

    while (offset < file_list_length) {
        uint16_t magic = *(uint16_t*)((uint32_t)ramdisk + offset);
        if (magic == FILE_ENTRY_MAGIC) {
            //if this is the last item in the path, then we found the file
            ramdisk_file_entry_t *file = (ramdisk_file_entry_t*)((uint32_t)ramdisk + offset);
            if (item == length && strcmp(buf, file->name) == 0) {
                ramdisk_dir_t *ret = (ramdisk_dir_t*)kmalloc(sizeof(ramdisk_dir_t));
                ret->flags = FILE_NOTFOUND_FLAG | FILE_ISFILE_FLAG; //tried to open file as a directory
                return ret;
            } else {
                offset += sizeof(ramdisk_file_entry_t);
            }
            //otherwise, we need to go deeper
        } else if (magic == DIR_ENTRY_MAGIC) {
            //next 64 bytes are the name of the directory
            offset += 2;
            if (strcmp(buf, (char*)((uint32_t)ramdisk + offset)) == 0) {
                //we found the directory
                if (item == length) {
                    //Found the directory!
                    ramdisk_dir_t *ret = (ramdisk_dir_t*)kmalloc(sizeof(ramdisk_dir_t));
                    ret->flags = FILE_ISOPENDIR_FLAG;
                    ret->idx = 0;
                    ret->num_files = *(uint32_t*)((uint32_t)ramdisk + offset + 64);
                    ret->files = (void*)((uint32_t)ramdisk + offset + 64 + 4);
                    memcpy(ret->name, buf, 64);
                    return ret;
                }
                item++;
                offset += 64;
                offset += 4;
                get_path_item(path, buf, item);
            } else {
                //we didn't find the directory, so skip it
                offset += 64;
                offset += 4;

                //loop until we find the end of the directory
                uint32_t depth = 1;
                while (depth > 0) {
                    if (*(uint16_t*)((uint32_t)ramdisk + offset) == DIR_ENTRY_MAGIC) {
                        depth++;
                        offset += 2;
                        offset += 64;
                        offset += 4;
                    } else if (*(uint16_t*)((uint32_t)ramdisk + offset) == DIR_END_MAGIC) {
                        depth--;
                        offset += 2;
                    } else if (*(uint16_t*)((uint32_t)ramdisk + offset) == FILE_ENTRY_MAGIC) {
                        offset += sizeof(ramdisk_file_entry_t);
                    } else {
                        kpanic("Invalid magic number: %x\n", *(uint16_t*)((uint32_t)ramdisk + offset));
                    }
                }
            }
        } else if (magic == DIR_END_MAGIC) {
            //we found the end of the directory, so we didn't find the file
            ramdisk_dir_t *ret = (ramdisk_dir_t*)kmalloc(sizeof(ramdisk_dir_t));
            ret->flags = FILE_NOTFOUND_FLAG;
            return ret;
        } else {
            kpanic("Invalid magic number: %x\n", magic);
        }
    }

    ramdisk_dir_t *ret = (ramdisk_dir_t*)kmalloc(sizeof(ramdisk_dir_t));
    ret->flags = FILE_NOTFOUND_FLAG;
    return ret;
}

int rread(void *ptr, uint32_t size, uint32_t nmemb, ramdisk_file_t *file) {
    if (file->flags & FILE_NOTFOUND_FLAG) {
        return 0;
    }
    uint32_t total_bytes = size * nmemb;
    uint32_t bytes_left = file->length - file->seek_pos;
    uint32_t bytes_to_read = total_bytes;
    if (bytes_left < total_bytes) {
        bytes_to_read = bytes_left;
    }
    memcpy(ptr, (void*)(file->addr + file->seek_pos), bytes_to_read);
    file->seek_pos += bytes_to_read;
    return bytes_to_read;
}

simple_return_t rreaddir(ramdisk_dir_t *dir) {
    if (dir->flags & FILE_NOTFOUND_FLAG) {
        simple_return_t ret;
        ret.flags = FILE_NOTFOUND_FLAG;
        return ret;
    }
    if (dir->idx >= dir->num_files) {
        simple_return_t ret;
        ret.flags = FILE_NOTFOUND_FLAG;
        return ret;
    }

    //search through to find an item
    uint32_t offset = 0;
    uint32_t depth = 0;
    uint32_t current_read_items = 0;
    while (true) {
        uint16_t magic = *(uint16_t*)((uint32_t)dir->files + offset);
        if (magic == FILE_ENTRY_MAGIC) {
            if (current_read_items == dir->idx) {
                ramdisk_file_entry_t *file = (ramdisk_file_entry_t*)((uint32_t)dir->files + offset);
                dir->idx++;
                simple_return_t ret;
                ret.flags = FILE_ISFILE_FLAG;
                ret.name = file->name;
                return ret;
            }
            current_read_items++;
            offset += sizeof(ramdisk_file_entry_t);
        } else if (magic == DIR_ENTRY_MAGIC) {
            if (current_read_items == dir->idx) {
                dir->idx++;
                simple_return_t ret;
                ret.flags = FILE_ISDIR_FLAG;
                ret.name = (char*)((uint32_t)dir->files + offset + 2);
                return ret;
            }
            current_read_items++;
            offset += 2;
            offset += 64;
            offset += 4;
            depth++;
        } else if (magic == DIR_END_MAGIC) {
            depth--;
            offset += 2;
        } else {
            kwarn("Invalid magic number: %x\n", magic);
            simple_return_t ret;
            ret.flags = FILE_NOTFOUND_FLAG;
            return ret;
        }
    }
    simple_return_t ret;
    ret.flags = FILE_NOTFOUND_FLAG;
    return ret;
}

int rseek(ramdisk_file_t *file, uint32_t offset, uint8_t whence) {
    if (file->flags & FILE_NOTFOUND_FLAG) {
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
    return 0;
}

int rtell(ramdisk_file_t *file) {
    if (file->flags & FILE_NOTFOUND_FLAG) {
        return -1;
    }
    return file->seek_pos;
}

int rclose(ramdisk_file_t *file) {
    if (file->flags & FILE_NOTFOUND_FLAG) {
        return -1;
    }
    file->flags &= ~FILE_ISOPEN_FLAG;
    kfree(file);
    return 0;
}

int rclosedir(ramdisk_dir_t *dir) {
    if (dir->flags & FILE_NOTFOUND_FLAG) {
        return -1;
    }
    dir->flags &= ~FILE_ISOPENDIR_FLAG;
    kfree(dir);
    return 0;
}

uint32_t getsize(void *fd) {
    if (*(uint32_t*)fd & FILE_ISFILE_FLAG) {
        return ((ramdisk_file_t*)fd)->length;
    } else {
        return ((ramdisk_dir_t*)fd)->num_files;
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

    serial_printf("Ramdisk address: %x\n", ramdisk_addr_multiboot);

    kassert(ramdisk_addr_multiboot != 0);

    ramdisk = (ramdisk_header_t*)(ramdisk_addr_multiboot + 0xC0000000);

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