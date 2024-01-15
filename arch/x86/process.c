#include <stdint.h>

#include "inc_c/display.h"
#include "inc_c/hardware.h"
#include "inc_c/string.h"
#include "../../kernel/include/filesystem.h"
#include "inc_c/memory.h"
#include "inc_c/serial.h"
#include "inc_c/process.h"
#include "inc_c/arch_elf.h"
#include "../../kernel/include/errors.h"
#include "inc_c/io.h"

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

uint32_t interrupt_esp;
uint32_t interrupt_ebp;

process_t kernel_process;

extern page_directory_t kernel_pd;   // kernel page directory
extern page_directory_t *current_pd; // current page directory

void serial_dump_process() {
    process_t *current_process = head_process;
    while (current_process != NULL) {
        serial_printf("Process: 0x%x\n", current_process);
        serial_printf("  PID: 0x%x\n", current_process->pid);
        serial_printf("  ESP: 0x%x\n", current_process->esp);
        serial_printf("  EBP: 0x%x\n", current_process->ebp);
        serial_printf("  Entry: 0x%x\n", current_process->entry);
        serial_printf("  Next: 0x%x\n", current_process->next);
        serial_printf("  PD: 0x%x\n", current_process->pd);
        serial_printf("  Status: 0x%x\n", current_process->status);
        serial_printf("  FDS: 0x%x\n", current_process->fds);
        serial_printf("  Num FDS: 0x%x\n", current_process->num_fds);
        serial_printf("  Max FDS: 0x%x\n", current_process->max_fds);
        current_process = current_process->next;
    }
}

void process_initialize()
{
    head_process = &kernel_process;
    head_process->pid = next_pid++;
    head_process->next = NULL;
    head_process->num_fds = 0;
    head_process->max_fds = 256;
    head_process->pd = &kernel_pd;
    head_process->status = TASK_STATUS_RUNNING;
    memset(head_process->fds, 0, sizeof(head_process->fds));
    current_process = head_process;

    terminal_printf("Address of head process' next field: 0x%x\n", &head_process->next);
}

void create_task(void *entry_point, void *stack) {
    process_t *new_process = (process_t *)kmalloc(sizeof(process_t));

    new_process->pid = next_pid++;
    new_process->next = NULL;
    new_process->num_fds = 0;
    new_process->max_fds = 256;
    new_process->pd = clone_page_directory(current_pd);
    new_process->status = TASK_STATUS_INITIALIZED;
    new_process->esp = (uint32_t)stack;
    new_process->ebp = (uint32_t)stack;
    memset(new_process->fds, 0, sizeof(new_process->fds));

    // Add to linked list
    process_t *current_process = head_process;
    while (current_process->next != NULL) {
        current_process = current_process->next;
    }
    current_process->next = new_process;

    new_process->entry = (uint32_t)entry_point;
}

extern void jump_to_program(uint32_t esp, uint32_t ebp);
extern void init_program_and_jump(uint32_t esp, uint32_t entry_point);

void timer_interrupt_handler(uint32_t ebp, uint32_t esp)
{
	outb(0x20, 0x20); //this isn't a regular IRQ handler, this is coming directly tables_asm, so we need to send an EOI to the PIC

	process_t *old_process = current_process;

	if (old_process->status == TASK_STATUS_RUNNING)
	{
        // Already running, so save context
		old_process->ebp = ebp;
		old_process->esp = esp;
	}

	process_t *new_process = old_process->next;
    if (new_process == NULL)
    {
        new_process = head_process; // If we ran off the end of the list, go back to the beginning
    }
    while (new_process->status != TASK_STATUS_RUNNING && new_process->status != TASK_STATUS_INITIALIZED)
    {
        new_process = new_process->next;
        if (new_process == NULL)
        {
            new_process = head_process; // If we ran off the end of the list, go back to the beginning
        }
    }

    current_process = new_process;

	if (new_process->status == TASK_STATUS_INITIALIZED)
	{
		new_process->status = TASK_STATUS_RUNNING;
        // First time running, so set up stack and jump to entry point
		init_program_and_jump(new_process->esp, new_process->entry);
	}

    // Otherwise, we're already running, so just restore context and return
	jump_to_program(new_process->esp, new_process->ebp);

	kpanic("Something went wrong with the scheduler!");
}


int process_load_elf(char *path) {
    
}