#ifndef _PROCESS_H
#define _PROCESS_H

#include <stdint.h>

#include "inc_c/hardware.h"
#include "../../kernel/include/filesystem.h"
#include "inc_c/memory.h"

typedef struct process {
    int pid;
    volatile uint32_t status;
    uint32_t stack_pos;
    uint32_t stack_size;
    uint32_t esp, ebp;
    uint32_t entry_or_return;
    file_descriptor_t *fds[256];
    uint32_t num_fds;
    uint32_t max_fds;
    struct process *next;
    page_directory_t *pd;

    // Things to pass to the process
    int argc;
    char **argv;
    char **envp;
} process_t;

#define TASK_STATUS_INITIALIZED 0
#define TASK_STATUS_RUNNING 1
#define TASK_STATUS_WAITING 2
#define TASK_STATUS_STOPPED 3
#define TASK_STATUS_FINISHED 4
#define TASK_STATUS_FORKED 5

void process_initialize();
process_t *process_load_elf(char *path);
process_t *create_task(void *entry_point, uint32_t stack_size, page_directory_t *pd, int argc, char **argv, char **envp);
void free_process(process_t *process);
uint32_t fork();

extern process_t *head_process;
extern process_t *current_process;

#endif