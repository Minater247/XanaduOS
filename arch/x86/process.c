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
        serial_printf("  Entry: 0x%x\n", current_process->entry_or_return);
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
}

process_t *create_task(void *entry_point, uint32_t stack_size) {
    process_t *new_process = (process_t *)kmalloc(sizeof(process_t));

    asm volatile ("cli");

    uint32_t stack = (uint32_t)kmalloc(stack_size) + stack_size;

    new_process->pid = next_pid++;
    new_process->next = NULL;
    new_process->num_fds = 0;
    new_process->max_fds = 256;
    new_process->pd = clone_page_directory(current_pd);
    new_process->status = TASK_STATUS_INITIALIZED;
    new_process->esp = (uint32_t)stack;
    new_process->ebp = (uint32_t)stack;
    new_process->stack_pos = stack;
    new_process->stack_size = stack_size;
    memset(new_process->fds, 0, sizeof(new_process->fds));
    for (int i = 0; i < 256; i++) {
        if (current_process->fds[i] != NULL) {
            new_process->fds[i] = copy_descriptor(current_process->fds[i], i);
        }
    }

    // Add to linked list
    process_t *current_process = head_process;
    while (current_process->next != NULL) {
        current_process = current_process->next;
    }
    current_process->next = new_process;

    new_process->entry_or_return = (uint32_t)entry_point;

    asm volatile ("sti");

    return new_process;
}

void free_process(process_t *process) {
    process_t *current_process = head_process;
    while (current_process->next != process) {
        current_process = current_process->next;
    }
    current_process->next = process->next;

    //close all file descriptors
    for (int i = 0; i < 256; i++) {
        if (process->fds[i] != NULL) {
            fclose(process->fds[i]);
        }
    }

    //free the stack
    kfree((void *)(process->stack_pos - process->stack_size));

    //free the page directory
    free_page_directory(process->pd);

    process->status = TASK_STATUS_FINISHED;

    //We leave it up to the creator of the process to free the process struct itself

    while (true);
}

process_t *process_by_pid(int pid) {
    process_t *wanted_process = head_process;
    while (wanted_process != NULL) {
        if (wanted_process->pid == pid) {
            return wanted_process;
        }
        wanted_process = wanted_process->next;
    }
    return NULL;
}

extern void jump_to_program(uint32_t esp, uint32_t ebp);

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

    switch_page_directory(new_process->pd);

	if (new_process->status == TASK_STATUS_INITIALIZED)
	{
		new_process->status = TASK_STATUS_RUNNING;
        // First time running, so set up stack and jump to entry point

        asm volatile ("mov %0, %%esp" : : "r" (new_process->esp));
        asm volatile ("mov %0, %%ebp" : : "r" (new_process->ebp));
        asm volatile ("sti");
        //set the function up as an function returning an int
        int (*entry_ptr)(void) = (int (*)(void))new_process->entry_or_return;
        //call the function
        int return_code = entry_ptr();
        asm volatile ("cli");
        new_process->entry_or_return = return_code;
        //remove the process from the scheduler
        free_process(new_process);
        kpanic("Something went wrong with the scheduler!");
	}

    // Otherwise, we're already running, so just restore context and return
	jump_to_program(new_process->esp, new_process->ebp);

	kpanic("Something went wrong with the scheduler!");
}


process_t *process_load_elf(char *path) {
	file_descriptor_t *fd = fopen(path, 0);
	if (fd->flags & FILE_NOTFOUND_FLAG || !(fd->flags & FILE_ISOPEN_FLAG))
	{
		terminal_printf("Could not locate %s!\n", path);
		while (true);
	}

	stat_t stat;
	int stat_ret = fstat(fd, &stat);
	if (stat_ret != 0)
	{
		terminal_printf("Could not stat %s!\n", path);
		while (true);
	}
	int size = stat.st_size;

	terminal_printf("Size of %s: %d\n", path, size);
	char *elfbuf = kmalloc(size);
	int read = fread(elfbuf, 1, size, fd);
	if (read != size)
	{
		terminal_printf("Could not read %s!\n", path);
		while (true);
	}
	terminal_printf("Read %d bytes from %s\n", read, path);

	fclose(fd);
	
	elf_load_result_t loaded = elf_load_executable(elfbuf);
	if (loaded.code != 0)
	{
		terminal_printf("Could not load %s! Error code: %d\n", path, loaded.code);
		while (true);
	}
	kfree(elfbuf);

    process_t *new_process = create_task((void *)loaded.entry_point, 0x1000);
    
    return new_process;
}