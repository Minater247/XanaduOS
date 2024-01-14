#ifndef _PROCESS_H
#define _PROCESS_H

#include <stdint.h>

#include "inc_c/hardware.h"
#include "../../kernel/include/filesystem.h"
#include "inc_c/memory.h"

typedef struct process {
    uint32_t pid;
    uint32_t status;
    uint32_t esp, ebp;
    uint32_t entry;
    file_descriptor_t *fds[256];
    uint32_t num_fds;
    uint32_t max_fds;
    struct process *next;
    page_directory_t *pd;
} process_t;

#define TASK_STATUS_INITIALIZED 0
#define TASK_STATUS_RUNNING 1
#define TASK_STATUS_WAITING 2
#define TASK_STATUS_STOPPED 3

void process_initialize();
int process_load_elf(char *path);

extern process_t *head_process;
extern process_t *current_process;

#endif