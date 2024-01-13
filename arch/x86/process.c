#include <stdint.h>

#include "inc_c/display.h"
#include "inc_c/hardware.h"
#include "inc_c/string.h"
#include "../../kernel/include/filesystem.h"
#include "inc_c/memory.h"
#include "inc_c/serial.h"
#include "inc_c/process.h"

// typedef struct process {
//     uint32_t pid;
//     regs_t regs;
//     file_descriptor_t *fds[256];
//     uint32_t num_fds;
//     uint32_t max_fds;
//     struct process *next;
// } process_t;

process_t *head_process = NULL;
process_t *current_process = NULL;
uint32_t next_pid = 0;



void serial_dump_process(process_t *to_dump) {
    serial_printf("PROCESS DUMP @ 0x%x\n", to_dump);

    serial_printf("\tPID: %d\n", to_dump->pid);
    serial_printf("\tREG: 0x%x\n", &to_dump->regs);
    serial_printf("\tFDS: 0x%x\n", to_dump->fds);
    for (uint32_t i = 0; i < to_dump->num_fds; i++) {
        serial_printf("\t\t0x%x\n", to_dump->fds[i]);
    }
    serial_printf("\tNFD: %d\n", to_dump->num_fds);
    serial_printf("\tNXT: 0x%x\n", to_dump->next);
}


void process_initialize() {
    head_process = (process_t *)kmalloc(sizeof(process_t));
    head_process->pid = next_pid++;
    head_process->next = NULL;
    head_process->num_fds = 0;
    head_process->max_fds = 256;
    memset(head_process->fds, 0, sizeof(head_process->fds));
    current_process = head_process;

    serial_dump_process(head_process);

    terminal_printf("Initialized process system.\n");
}