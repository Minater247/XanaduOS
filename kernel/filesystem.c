#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "include/errors.h"
#include "inc_c/string.h"
#include "include/filesystem.h"
#include "inc_c/memory.h"
#include "inc_c/process.h"

fs_node_t head_node = {NULL, NULL};
mount_point_t head_mount_point = {NULL, NULL, NULL};
uint32_t fs_id = 0;
uint32_t file_id = 0;

void get_path_item(char *path, char *retbuf, uint8_t item) {
    uint8_t i = 0;
    uint8_t j = 0;
    uint8_t item_num = 0;
    while (path[i] == '/') {
        i++;
    }
    while (path[i] != '\0') {
        if (path[i] == '/') {
            if (item_num == item) {
                retbuf[j] = '\0';
                return;
            }
            item_num++;
            j = 0;
        } else {
            retbuf[j] = path[i];
            j++;
        }
        i++;
    }
    retbuf[j] = '\0';
}

uint32_t get_path_length(char *path) {
    uint32_t i = 0;
    uint32_t length = 0;
    while (path[i] == '/') {
        i++;
    }
    if (path[i] != '\0') {
        length++;
    }
    while (path[i] != '\0') {
        if (path[i] == '/' && !(path[i + 1] == '\0' || path[i + 1] == '/')) {
            length++;
        }
        i++;
    }
    return length;
}

uint32_t next_id() {
    //if we've used all the ids, panic
    if (fs_id == 0xFFFFFFFE) {
        kpanic("Out of filesystem ids!\n");
    }
    fs_id++;
    return fs_id;
}

uint32_t next_file_id() {
    //if we've used all the ids, panic
    if (current_process->num_fds >= current_process->max_fds) {
        kpanic("Process %d out of file descriptors!\n", current_process->pid);
    }
    //find the first NULL file descriptor
    for (uint32_t i = 0; i < current_process->max_fds; i++) {
        if (current_process->fds[i] == NULL) {
            current_process->num_fds++;
            return i;
        }
    }
    //if we get here, something went wrong, that should already be checked for
    kpanic("Something went wrong in next_file_id!\n");
}

int register_filesystem(filesystem_t *to_register) {
    to_register->identifier = next_id();
    if (head_node.fs == NULL) {
        head_node.fs = to_register;
        head_node.next = NULL;
        return (int)to_register->identifier;
    } else {
        fs_node_t *cur_node = &head_node;
        while (cur_node->next != NULL) {
            cur_node = cur_node->next;
        }
        cur_node->next = (fs_node_t*)kmalloc(sizeof(fs_node_t));
        cur_node->next->fs = to_register;
        cur_node->next->next = NULL;
        return (int)to_register->identifier;
    }
}

int unregister_filesystem(uint32_t filesystem_id) {
    fs_node_t *cur_node = &head_node;
    fs_node_t *prev_node = NULL;
    while (cur_node != NULL) {
        if (cur_node->fs->identifier == filesystem_id) {
            break;
        }
        prev_node = cur_node;
        cur_node = cur_node->next;
    }
    if (cur_node == NULL) {
        return -1;
    }
    if (prev_node == NULL) {
        head_node = *cur_node->next;
    } else {
        prev_node->next = cur_node->next;
    }
    kfree(cur_node);
    return 0;
}

int mount_filesystem(uint32_t filesystem_id, char *path) {
    fs_node_t *cur_node = &head_node;
    while (cur_node != NULL) {
        if (cur_node->fs->identifier == filesystem_id) {
            break;
        }
        cur_node = cur_node->next;
    }
    if (cur_node == NULL) {
        return -1;
    }
    while (path[0] == '/') {
        path++;
    }
    if (head_mount_point.path == NULL) {
        head_mount_point.path = path;
        head_mount_point.fs = cur_node->fs;
        head_mount_point.next = NULL;
        return 0;
    } else {
        mount_point_t *cur_mount_point = &head_mount_point;
        while (cur_mount_point->next != NULL) {
            cur_mount_point = cur_mount_point->next;
        }
        cur_mount_point->next = (mount_point_t*)kmalloc(sizeof(mount_point_t));
        cur_mount_point->next->path = path;
        cur_mount_point->next->fs = cur_node->fs;
        cur_mount_point->next->next = NULL;
        return 0;
    }
}

int unmount_filesystem_by_id(uint32_t filesystem_id) {
    mount_point_t *cur_mount_point = &head_mount_point;
    mount_point_t *prev_mount_point = NULL;
    while (cur_mount_point != NULL) {
        if (cur_mount_point->fs->identifier == filesystem_id) {
            break;
        }
        prev_mount_point = cur_mount_point;
        cur_mount_point = cur_mount_point->next;
    }
    if (cur_mount_point == NULL) {
        return -1;
    }
    if (prev_mount_point == NULL) {
        head_mount_point = *cur_mount_point->next;
    } else {
        prev_mount_point->next = cur_mount_point->next;
    }
    kfree(cur_mount_point);
    return 0;
}

int unmount_filesystem_by_path(char *path) {
    mount_point_t *cur_mount_point = &head_mount_point;
    mount_point_t *prev_mount_point = NULL;
    while (cur_mount_point != NULL) {
        if (!strcmp(cur_mount_point->path, path)) {
            break;
        }
        prev_mount_point = cur_mount_point;
        cur_mount_point = cur_mount_point->next;
    }
    if (cur_mount_point == NULL) {
        return -1;
    }
    if (prev_mount_point == NULL) {
        head_mount_point = *cur_mount_point->next;
    } else {
        prev_mount_point->next = cur_mount_point->next;
    }
    kfree(cur_mount_point);
    return 0;
}

file_descriptor_t *fopen(char *path, char *flags) {
    //later on we will check for the PWD, but for now only accept absolute paths
    if (path[0] != '/') {
        file_descriptor_t *ret = (file_descriptor_t*)kmalloc(sizeof(file_descriptor_t));
        ret->flags = FILE_NOTFOUND_FLAG;
        ret->fs = NULL;
        return ret;
    }

    // Check whether this path matches a mount point
    mount_point_t *cur_mount_point = &head_mount_point;
    while (path[0] == '/') {
        path++;
    }
    
    int len;
    while (cur_mount_point != NULL) {
        len = strlen(cur_mount_point->path);
        if (!strncmp(path, cur_mount_point->path, len)) {
            path += len;
            break;
        }
        cur_mount_point = cur_mount_point->next;
    }
    if (cur_mount_point == NULL) {
        //TODO: allow setting up default filesystem for non-mounted paths
        file_descriptor_t *ret = (file_descriptor_t*)kmalloc(sizeof(file_descriptor_t));
        ret->flags = FILE_NOTFOUND_FLAG;
        ret->fs = NULL;
        return ret;
    }

    file_descriptor_t *ret = (file_descriptor_t*)kmalloc(sizeof(file_descriptor_t));
    filesystem_t *fs = cur_mount_point->fs;
    ret->fs = fs;
    ret->fs_data = fs->open(path, flags);
    ret->flags = *(uint32_t*)ret->fs_data;
    ret->id = next_file_id();
    current_process->fds[ret->id] = ret;

    return ret;
}

file_descriptor_t *copy_descriptor(file_descriptor_t *fd, uint32_t id) {
    file_descriptor_t *ret = (file_descriptor_t*)kmalloc(sizeof(file_descriptor_t));
    ret->fs = fd->fs;
    if (fd->fs_data != NULL) {
        ret->fs_data = fd->fs->copy(fd->fs_data);
    } else {
        ret->fs_data = NULL;
    }
    ret->flags = fd->flags;
    ret->id = id;
    return ret;
}

int fread(char *buf, size_t size, size_t count, file_descriptor_t *fd) {
    return fd->fs->read(buf, size, count, fd->fs_data);
}

int fwrite(char *buf, size_t size, size_t count, file_descriptor_t *fd) {
    fd->flags |= FILE_WRITTEN_FLAG;
    return fd->fs->write(buf, size, count, fd->fs_data);
}

int fclose(file_descriptor_t *fd) {
    if (fd->fs == NULL) {
        return 0;
    }

    if (fd->fs->close == NULL) {
        return 0;
    }

    if ((int)fd == -1) {
        return 0;
    }

    if (fd->flags & FILE_ISOPENDIR_FLAG) {
        return fclosedir((dir_descriptor_t *)fd);
    }

    if (!(fd->flags & FILE_ISOPEN_FLAG)) {
        return 0;
    }

    // Clear the file descriptor
    current_process->fds[fd->id] = NULL;
    current_process->num_fds--;

    return fd->fs->close(fd->fs_data);
}

dir_descriptor_t *fopendir(char *path) {
    //later on we will check for the PWD, but for now only accept absolute paths
    if (path[0] != '/') {
        dir_descriptor_t *ret = (dir_descriptor_t*)kmalloc(sizeof(dir_descriptor_t));
        ret->flags = FILE_NOTFOUND_FLAG;
        ret->fs = NULL;
        return ret;
    }

    //get the mount point
    mount_point_t *cur_mount_point = &head_mount_point;
    while (path[0] == '/') {
        path++;
    }
    int len;
    while (cur_mount_point != NULL) {
        len = strlen(cur_mount_point->path);
        if (!strncmp(path, cur_mount_point->path, len - 1)) { //len-1 to prevent #GP
            path += len;
            break;
        }
        cur_mount_point = cur_mount_point->next;
    }
    if (cur_mount_point == NULL) {
        //TODO: allow setting up default filesystem for non-mounted paths
        dir_descriptor_t *ret = (dir_descriptor_t*)kmalloc(sizeof(dir_descriptor_t));
        ret->flags = FILE_NOTFOUND_FLAG;
        ret->fs = NULL;
        return ret;
    }

    dir_descriptor_t *ret = (dir_descriptor_t*)kmalloc(sizeof(dir_descriptor_t));
    filesystem_t *fs = cur_mount_point->fs;
    ret->fs = fs;
    ret->id = next_file_id();
    ret->fs_data = fs->opendir(path);
    ret->flags = *(uint32_t*)ret->fs_data;
    current_process->fds[ret->id] = (file_descriptor_t *)ret;
    return ret;
}

int fclosedir(dir_descriptor_t *dd) {
    return dd->fs->closedir(dd->fs_data);
}

int fseek(file_descriptor_t *fd, size_t offset, int whence) {
    return fd->fs->seek(fd->fs_data, offset, whence);
}

size_t ftell(file_descriptor_t *fd) {
    return fd->fs->tell(fd->fs_data);
}

int fstat(file_descriptor_t *fd, stat_t *statbuf) {
    return fd->fs->stat(fd->fs_data, statbuf);
}

dirent_t *fgetdent(dirent_t *buf, uint32_t entry_num, dir_descriptor_t *fd) {
    return fd->fs->getdent(buf, entry_num, fd->fs_data);
}