#include "scheduler.h"
#include "kalloc.h"
#include "vm.h"
#include "string.h"
#include "main.h"

extern unsigned long* root_table;
extern void  _trigger_smode_software_interrupt(void);
extern void _set_ssie(void);
extern void _off_ssie(void);

struct process* curr_process = 0;
struct process* active_process_list_head = 0;
struct process* blocked_process_list_head = 0;
struct process* zombie_process_list_head = 0;

unsigned long pid_counter = 0;

void scheduler_init(void){
	curr_process = (struct process*)kvmalloc(sizeof(struct process));
	curr_process->pid = pid_counter;
	pid_counter++;
	curr_process->state = 1;
	char* name = "oxomoco";
	for (unsigned int i = 0; i < kstrlen(name); i++){
		curr_process->name[i] = name[i];
	}
	curr_process->name[kstrlen(name)] = '\0';
	curr_process->satp = (unsigned long)root_table;
	curr_process->magic = 0xC00C33E1;
	curr_process->next = curr_process;
	curr_process->prev = curr_process;
	active_process_list_head = curr_process;
	_load_sscratch((unsigned long)curr_process);
	return;
}

void round_robin(void){

	if (curr_process->magic != 0xC00C33E1) kpanic(114,curr_process->pid);

	if (curr_process->state == 1) curr_process->state = 0;

	if (curr_process->state == 2 || curr_process->state == 3){
		curr_process = active_process_list_head;
	} else{
		curr_process = curr_process->next;
	}

	while (curr_process->magic != 0xC00C33E1 || curr_process->state != 0) curr_process = curr_process->next;
		
	curr_process->state = 1;
	return;
}

struct process* spawn_process(void (*entry_function)(void), char* name, unsigned long sstatus_val, unsigned long satp_val){
	struct process* new_process = (struct process*)kvmalloc(sizeof(struct process));

	if (!new_process) return 0;

	new_process->pid = pid_counter;
	pid_counter++;
	new_process->state = 0;
	
	for (int i = 0; i < 32; i++){
		new_process->context[i] = 0;
	}

	unsigned long* process_stack = (unsigned long*)kalloc(0);
	
	if (!process_stack) return 0;

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

	new_process->next = active_process_list_head;
	struct process* tail = active_process_list_head->prev;
	new_process->prev = tail;
	tail->next = new_process;
	active_process_list_head->prev = new_process;
	active_process_list_head = new_process;

	return new_process;
}

void block_process(struct process* target){
	_off_ssie();

	if(target->pid == 0) kpanic(113,target->sepc);

	if(target->magic != 0xC00C33E1) kpanic(114,target->pid);

	struct process* head = target->next;
	struct process* tail = target->prev;

	head->prev = tail;
	tail->next = head;

	if (target == active_process_list_head){
		active_process_list_head = target->next;
		active_process_list_head->prev = target->prev;
	}

	target->next = blocked_process_list_head;
	
	if(blocked_process_list_head != 0) blocked_process_list_head->prev = target;

	target->prev = 0;

	blocked_process_list_head = target;

	target->state = 2;
	
	_set_ssie();
	_trigger_smode_software_interrupt();

	return;
}

void unblock_process(struct process* target){
	
	if(target->state != 2) kpanic(115,target->pid);

	if(target->magic != 0xC00C33E1) kpanic(114,target->pid);

	target->state = 0;

	if (target == blocked_process_list_head){
		blocked_process_list_head = target->next;

		if (blocked_process_list_head != 0) blocked_process_list_head->prev = 0;

	} else {
		struct process* head = target->next;
		struct process* tail = target->prev;

		head->prev = tail;
		tail->next = head;
	}
	
	target->next = active_process_list_head;
	struct process* tail = active_process_list_head->prev;
	target->prev = tail;
	tail->next = target;
	active_process_list_head->prev = target;
	active_process_list_head = target;

	_trigger_smode_software_interrupt();

	return;
}

void exit_process(unsigned long code){
	_off_ssie();

	if(curr_process->pid == 0) kpanic(113,curr_process->sepc);

	if(curr_process->magic != 0xC00C33E1) kpanic(114,curr_process->pid);

	struct process* head = curr_process->next;
	struct process* tail = curr_process->prev;

	head->prev = tail;
	tail->next = head;

	if (curr_process == active_process_list_head){
		active_process_list_head = curr_process->next;
		active_process_list_head->prev = curr_process->prev;
	}

	curr_process->next = zombie_process_list_head;
	
	if(zombie_process_list_head != 0) zombie_process_list_head->prev = curr_process;

	curr_process->prev = 0;

	zombie_process_list_head = curr_process;

	curr_process->state = 3;
	curr_process->exit_code = code;

	struct process* ptr = active_process_list_head;

	if (ptr->parent_pid == curr_process->pid){
		ptr->parent_pid = 0;
	}

	ptr = ptr->next;

	while (ptr != active_process_list_head){
		if (ptr->parent_pid == curr_process->pid){
			ptr->parent_pid = 0;
		}
		ptr = ptr->next;
	}
	
	ptr = blocked_process_list_head;

	while (ptr != 0){
		if (ptr->parent_pid == curr_process->pid){
			ptr->parent_pid = 0;
		}
		ptr = ptr->next;
	}
	
	ptr = zombie_process_list_head;

	while (ptr != 0){
		if (ptr->parent_pid == curr_process->pid){
			ptr->parent_pid = 0;
		}
		ptr = ptr->next;
	}
	
	_set_ssie();
	_trigger_smode_software_interrupt();

	return;	
}

unsigned long wait_process(void){
	_off_ssie();

	struct process* header = zombie_process_list_head;

	while (header != 0 && header->parent_pid != curr_process->pid) header = header->next;

	if (header == 0){
		_set_ssie();
		return (unsigned long)-1;
	}

	if (header == zombie_process_list_head){
		zombie_process_list_head = header->next;
		if (zombie_process_list_head != 0) {
			zombie_process_list_head->prev = 0;
		}
	} else{
		struct process* head = header->next;
		struct process* tail = header->prev;

		head->prev = tail;
		tail->next = head;	
	}
	
	unsigned long code = header->exit_code;

	kfree(header->kstack);
	kvmfree(header);
	
	_set_ssie();
	return code;
}

void orphan_cleaner(void){
	struct process* ptr = zombie_process_list_head;
	
	while (ptr != 0){
		struct process* next_ptr = ptr->next;

		if (ptr->parent_pid == 0){
			if (ptr == zombie_process_list_head){
				zombie_process_list_head = ptr->next;
				if (zombie_process_list_head != 0) {
					zombie_process_list_head->prev = 0;
				}
			} else{
				struct process* head = ptr->next;
				struct process* tail = ptr->prev;

				head->prev = tail;
				tail->next = head;	
			}
			
			
			kfree(ptr->kstack);
			kvmfree(ptr);
		}

		ptr = next_ptr;
	}
	return;
}
