#ifndef _PROCESS_H
#define _PROCESS_H

#include <stdint.h>

#include "inc_c/hardware.h"
#include "../../kernel/include/filesystem.h"

typedef struct process {
    uint32_t pid;
    regs_t regs;
    file_descriptor_t *fds[256];
    uint32_t num_fds;
    uint32_t max_fds;
    struct process *next;
} process_t;

void process_initialize();

extern process_t *head_process;
extern process_t *current_process;

#endif