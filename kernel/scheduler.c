#include "scheduler.h"
#include "kalloc.h"
#include "vm.h"
#include "string.h"

extern unsigned long* root_table;

struct process* curr_process = 0;
struct process* process_list_head = 0;

unsigned long pid_counter = 0;

void scheduler_init(void){
	curr_process = (struct process*)kvmalloc(sizeof(struct process));
	curr_process->pid = pid_counter;
	pid_counter++;
	curr_process->state = 1;
	char* name = "oxomoco";
	for (int i = 0; i < kstrlen(name); i++){
		curr_process->name[i] = name[i];
	}
	curr_process->name[kstrlen(name)] = '\0';
	curr_process->satp = (unsigned long)root_table;
	curr_process->magic = 0xC00C33E1;
	curr_process->next = curr_process;
	curr_process->prev = curr_process;
	process_list_head = curr_process;
	_load_sscratch((unsigned long)curr_process);
	return;
}

void round_robin(void){
	curr_process->state = 0;
	curr_process = curr_process->next;

	while (curr_process->magic == 0xC00C33E1 && curr_process->state != 0) curr_process = curr_process->next;
	
	curr_process->state = 1;
	return;
}

void spawn_process(void (*entry_function)(void), char* name, unsigned long sstatus_val, unsigned long satp_val){
	struct process* new_process = (struct process*)kvmalloc(sizeof(struct process));

	if (!new_process) return;

	new_process->pid = pid_counter;
	pid_counter++;
	new_process->state = 0;
	
	for (int i = 0; i < 32; i++){
		new_process->context[i] = 0;
	}

	unsigned long* process_stack = (unsigned long*)kalloc(0);
	
	if (!process_stack) return;

	new_process->kstack = process_stack;

	unsigned long process_stack_base_addr = (unsigned long)process_stack + 4096;
	new_process->context[2] = process_stack_base_addr;

	int len = kstrlen(name);

	if(len > 23) len = 23;
	
	for (int i = 0; i < len; i++){
		new_process->name[i] = name[i];
	}
	new_process->name[len] = '\0';

	new_process->sstatus = sstatus_val;
	new_process->sepc = (unsigned long)entry_function;
	new_process->satp = satp_val;
	new_process->magic = 0xC00C33E1;

	new_process->next = process_list_head;
	struct process* tail = process_list_head->prev;
	new_process->prev = tail;
	tail->next = new_process;
	process_list_head->prev = new_process;
	process_list_head = new_process;

	return;
}
