#include "scheduler.h"
#include "kalloc.h"
#include "vm.h"
#include "string.h"
#include "main.h"
#include "smp.h"
#include "dtb.h"

extern char _stack_top[];

extern unsigned long* root_table;
extern struct process* shell_ptr;

extern void  _trigger_smode_software_interrupt(void);
extern void _set_ssie(void);
extern void _off_ssie(void);
extern void _switch_context(struct process* out_process, struct process* in_process);
extern void _process_bootstrap(void);
extern void _wfi(void);

extern struct cpu_closet* global_closet;

#define PID_MAX 32768

struct process* blocked_process_list_head = 0;
struct process* zombie_process_list_head = 0;
struct process* sleeping_process_list_head = 0;
struct process* blocked_ipc_process_list_head = 0;
struct process* blocked_ipc_send_process_list_head = 0;
struct process* emergency_process = 0;

unsigned long alive_thread_counter = 0;
unsigned long next_pid = 1;

unsigned long* pid_registry = 0;

void scheduler_init(void){
	(get_runqueue()->curr_process) = (struct process*)kvmalloc(sizeof(struct process));
	(get_runqueue()->curr_process)->pid = 0;
	spinlock_acquire(&alive_counter_lock);
	alive_thread_counter++;
	spinlock_release(&alive_counter_lock);
	(get_runqueue()->curr_process)->state = 1;
	char* name = "oxomoco";
	for (unsigned int i = 0; i < kstrlen(name); i++){
		(get_runqueue()->curr_process)->name[i] = name[i];
	}
	(get_runqueue()->curr_process)->name[kstrlen(name)] = '\0';
	(get_runqueue()->curr_process)->satp = (unsigned long)root_table;
	(get_runqueue()->curr_process)->capability_root = (unsigned long*)(kalloc(2));

	for (int i = 0; i < 2048; i++){
		(get_runqueue()->curr_process)->capability_root[i] = 0;
	}

	(get_runqueue()->curr_process)->tf = 0;
	(get_runqueue()->curr_process)->magic = 0xC00C33E1;
	(get_runqueue()->curr_process)->context[0] = (unsigned long) oxomoco_loop;
	(get_runqueue()->curr_process)->context[1] = (unsigned long) _stack_top;
	(get_runqueue()->curr_process)->next = (get_runqueue()->curr_process);
	(get_runqueue()->curr_process)->prev = (get_runqueue()->curr_process);
	(get_runqueue()->curr_process)->priority = 31;
	(get_runqueue()->active_process_circles)[(get_runqueue()->curr_process)->priority] = (get_runqueue()->curr_process);
	(get_runqueue()->lookup_bitmap) |= (1UL << (get_runqueue()->curr_process)->priority);
	(get_runqueue()->curr_process)->time_slice = (((get_runqueue()->curr_process)->priority << 2) + 4);
	(get_runqueue()->curr_process)->ticks_left = (get_runqueue()->curr_process)->time_slice;
	(get_runqueue()->curr_process)->cpu_time = 0;
	(get_runqueue()->idle_process) = (get_runqueue()->curr_process);
	pid_registry = (unsigned long*)(kalloc(0));
	
	for (int i = 0; i < 512; i++){
		pid_registry[i] = 0;
	}

	pid_registry[0] |= (1UL << 0);

	(get_runqueue()->process_count) = 1;

	_load_sscratch((unsigned long)(get_runqueue()->curr_process)->tf);
	return;
}

void schedule(void){
	spinlock_acquire(&(get_runqueue()->lock));
	struct process* out_process = (get_runqueue()->curr_process);

	if ((get_runqueue()->curr_process)->magic != 0xC00C33E1) kpanic(114,(get_runqueue()->curr_process)->pid);

	if ((get_runqueue()->curr_process)->state == 1){
		(get_runqueue()->curr_process)->state = 0;
		(get_runqueue()->curr_process)->ticks_left = (get_runqueue()->curr_process)->time_slice;
		(get_runqueue()->active_process_circles)[(get_runqueue()->curr_process)->priority] = (get_runqueue()->active_process_circles)[(get_runqueue()->curr_process)->priority]->next;
	}

	if (emergency_process != 0){
		(get_runqueue()->curr_process) = emergency_process;
		emergency_process = 0;
	} else {
	
		for (int i = 0; i < 32; i++){
			if ((get_runqueue()->lookup_bitmap) & (1UL << i)){
				(get_runqueue()->curr_process) = (get_runqueue()->active_process_circles)[i];
				
				if ((get_runqueue()->curr_process)->magic != 0xC00C33E1) kpanic(114,(get_runqueue()->curr_process)->pid);
				
				if (i < 31) (get_runqueue()->lookup_bitmap) |= ((((get_runqueue()->lookup_bitmap) & 0xFFFFFFFF) & (~((1UL << (i+1)) - 1 ))) << 32);
				
				break;
			}
		}
	}

	if ((get_runqueue()->curr_process)->state != 0) kpanic(114, (get_runqueue()->curr_process)->pid);

	(get_runqueue()->curr_process)->state = 1;

	struct process* in_process = (get_runqueue()->curr_process);

	spinlock_release(&(get_runqueue()->lock));

	_switch_context(out_process,in_process);

	return;
}

struct process* spawn_process(char* _binary, char* name, unsigned long sstatus_val, unsigned int priority){
	struct process* new_process = (struct process*)kvmalloc(sizeof(struct process));

	if (!new_process) return 0;
	
	if (alive_thread_counter >= PID_MAX) return 0;
	
	unsigned int idx = next_pid / 64;
	unsigned int offset = next_pid % 64;
	
	spinlock_acquire(&pid_registry_lock);
	if (next_pid < PID_MAX && ((pid_registry[idx]) & (1UL << offset)) == 0) {
		new_process->pid = next_pid;
		pid_registry[idx] |= (1UL << offset);
		spinlock_acquire(&alive_counter_lock);
		alive_thread_counter++;
		spinlock_release(&alive_counter_lock);
		next_pid++;
	} else {
		if (next_pid >= PID_MAX) next_pid = 1;
		
		while (((pid_registry[idx]) & (1UL << offset)) != 0){
			next_pid++;
			idx = next_pid / 64;
			offset = next_pid % 64;

			if (next_pid >= PID_MAX) next_pid = 1;
		}

		new_process->pid = next_pid;
		pid_registry[idx] |= (1UL << offset);
		spinlock_acquire(&alive_counter_lock);
		alive_thread_counter++;
		spinlock_release(&alive_counter_lock);
		next_pid++;
	}
	spinlock_release(&pid_registry_lock);

	new_process->state = 0;
	
	for (int i = 0; i < 14; i++){
		new_process->context[i] = 0;
	}

	unsigned long* process_stack = (unsigned long*)kalloc(0);
	
	if (!process_stack) return 0;

	new_process->kstack = process_stack;

	unsigned long process_stack_base_addr = (unsigned long)process_stack + 4096 - sizeof(struct trapframe);
	new_process->tf = (struct trapframe*)process_stack_base_addr;
	new_process->context[1] = process_stack_base_addr;
	new_process->context[0] = (unsigned long)_process_bootstrap;

	int len = kstrlen(name);

	if(len > 23) len = 23;
	
	for (int i = 0; i < len; i++){
		new_process->name[i] = name[i];
	}
	new_process->name[len] = '\0';

	unsigned long* trap_ptr = (unsigned long*) new_process->tf;
	for (int i = 0; i < (sizeof(struct trapframe)/8); i++){
		trap_ptr[i] = 0;
	}

	new_process->tf->sstatus = sstatus_val;
	new_process->tf->kernel_satp = (0x8000000000000000 | ((unsigned long)root_table >> 12));
	new_process->tf->kernel_trap = (unsigned long)strap_router;
	new_process->tf->kernel_sp = process_stack_base_addr;
	new_process->tf->kernel_tp = (unsigned long)get_runqueue();
	unsigned long* new_user_table = create_user_table(process_stack); 
	new_process->satp = (0x8000000000000000 | ((unsigned long)new_user_table >> 12));
	new_process->tf->user_satp = new_process->satp;
	new_process->magic = 0xC00C33E1;
	
	if (!(*(_binary) == 0x7F && *(_binary + 1) == 'E' && *(_binary + 2) == 'L' && *(_binary + 3) == 'F')) kpanic(116,new_process->pid);

	unsigned long e_phoff = *((unsigned long*)(_binary + 32));
	unsigned short e_phentsize = *((unsigned short*)(_binary + 54));
	unsigned short e_phnum = *((unsigned short*)(_binary + 56));

	struct elf64_phdr {
		unsigned int p_type; 
		unsigned int p_flags; 
		unsigned long p_offset; 
		unsigned long p_vaddr; 
		unsigned long p_paddr; 
		unsigned long p_filesz; 
		unsigned long p_memsz; 
		unsigned long p_align; 
	};

	unsigned char* ph_ptr = _binary + e_phoff; 

	for (unsigned short i = 0; i < e_phnum; i++){
		struct elf64_phdr* curr_ph_ptr = (struct elf64_phdr*)ph_ptr;
		
		if (curr_ph_ptr->p_type == 1){
			unsigned long mem_size = curr_ph_ptr->p_memsz;
			unsigned int order = 0;
			while ( mem_size > (4096 << order) ) order++;
			unsigned char* buff = (unsigned char*)kalloc(order);
			unsigned long file_size = curr_ph_ptr->p_filesz;
			unsigned char* file_ptr = _binary + curr_ph_ptr->p_offset;
			for (unsigned long j = 0;j < file_size; j++){
				*(buff + j) = *(file_ptr + j);
			}

			for (unsigned long j = file_size; j < mem_size; j++){
				*(buff + j) = 0x00;
			}

			unsigned int perm = 0;

			if ((curr_ph_ptr->p_flags & 0x1) && !(curr_ph_ptr->p_flags & 0x2)){
				perm += 8;
			} else if (curr_ph_ptr->p_flags & 0x2) {
				perm += 4;
			}

			if (curr_ph_ptr->p_flags & 0x4) perm += 2;
			
			uvm_map(new_user_table, curr_ph_ptr->p_vaddr, (unsigned long) buff, 4096 << order, perm);
		}

		ph_ptr += (unsigned long)(e_phentsize);
	}

	new_process->tf->epc = *((unsigned long*)(_binary + 24));
	unsigned long* ustack = (unsigned long*)kalloc(0);

	for (int i = 0; i < 512; i++) ustack[i] = 0x00;

	uvm_map(new_user_table, 0x7FFFF000, (unsigned long) ustack, 4096, 6);
	new_process->tf->u_context[2] = 0x80000000;

	new_process->parent_pid = ((get_runqueue()->curr_process) != 0) ? (get_runqueue()->curr_process)->pid : 0;
	new_process->mailbox_status = 0;

	new_process->capability_root = (unsigned long*)(kalloc(2));

	for (int i = 0; i < 2048; i++){
		new_process->capability_root[i] = 0;
	}

	new_process->capability_root[0] |= (3UL << (1 * 4));

	int n_idx = new_process->pid / 16;
	int n_offset = ((new_process->pid % 16) * 4);

	(get_runqueue()->curr_process)->capability_root[n_idx] |= (7UL << n_offset);

	if ((shell_ptr != 0) && (new_process->pid != 1)) shell_ptr->capability_root[n_idx] |= (15UL << n_offset);

	new_process->capability_root[n_idx] |= (1UL << n_offset);
	
	new_process->active_grant = 0;

	for (int i = 0; i < 8; i++) new_process->grant_whitelist[i] = 0;

	new_process->priority = priority;
	new_process->time_slice = ((priority << 2) + 4);
	new_process->ticks_left = new_process->time_slice;
	new_process->cpu_time = 0;
	
	spinlock_acquire(&(get_runqueue()->lock));
	if ((get_runqueue()->lookup_bitmap) & (1UL << priority)){
		new_process->next = (get_runqueue()->active_process_circles)[priority];
		struct process* tail = (get_runqueue()->active_process_circles)[priority]->prev;
		new_process->prev = tail;
		tail->next = new_process;
		(get_runqueue()->active_process_circles)[priority]->prev = new_process;
	} else {
		new_process->next = new_process;
		new_process->prev = new_process;	
		(get_runqueue()->active_process_circles)[priority] = new_process;
		(get_runqueue()->lookup_bitmap) |= (1UL << priority);
	}

	get_runqueue()->process_count++;
	spinlock_release(&(get_runqueue()->lock));

	return new_process;
}

void block_process(struct process* target){
	spinlock_acquire(&blocked_list_lock);

	if(target->pid == 0) kpanic(113,target->tf->epc);

	if(target->magic != 0xC00C33E1) kpanic(114,target->pid);

	if (emergency_process == target) emergency_process = 0;
	
	spinlock_acquire(&(get_runqueue()->lock));
	if (target->next == target){
		(get_runqueue()->active_process_circles)[target->priority] = 0;
		(get_runqueue()->lookup_bitmap) &= (~(1UL << target->priority));
	} else {
		struct process* head = target->next;
		struct process* tail = target->prev;

		head->prev = tail;
		tail->next = head;

		if (target == (get_runqueue()->active_process_circles)[target->priority]){
			(get_runqueue()->active_process_circles)[target->priority] = target->next;
		}
	}
	
	get_runqueue()->process_count--;
	spinlock_release(&(get_runqueue()->lock));
	target->next = blocked_process_list_head;
	
	if(blocked_process_list_head != 0) blocked_process_list_head->prev = target;

	target->prev = 0;

	blocked_process_list_head = target;

	target->state = 2;

	spinlock_release(&blocked_list_lock);
	
	_trigger_smode_software_interrupt();

	return;
}

void unblock_process(struct process* target){
	spinlock_acquire(&blocked_list_lock);
	
	if(target->state != 2) kpanic(115,target->pid);

	if(target->magic != 0xC00C33E1) kpanic(114,target->pid);

	target->state = 0;

	if (target == blocked_process_list_head){
		blocked_process_list_head = target->next;

		if (blocked_process_list_head != 0) blocked_process_list_head->prev = 0;

	} else {
		struct process* head = target->next;
		struct process* tail = target->prev;

		if (head != 0) head->prev = tail;
		tail->next = head;
	}
	
	spinlock_acquire(&(get_runqueue()->lock));	
	if ((get_runqueue()->lookup_bitmap) & (1UL << target->priority)){
		target->next = (get_runqueue()->active_process_circles)[target->priority];
		struct process* tail = (get_runqueue()->active_process_circles)[target->priority]->prev;
		target->prev = tail;
		tail->next = target;
		(get_runqueue()->active_process_circles)[target->priority]->prev = target;
	} else {
		target->next = target;
		target->prev = target;	
		(get_runqueue()->active_process_circles)[target->priority] = target;
		(get_runqueue()->lookup_bitmap) |= (1UL << target->priority);
	}
	target->tf->kernel_tp = (unsigned long)get_runqueue();
	get_runqueue()->process_count++;
	spinlock_release(&(get_runqueue()->lock));
	
	spinlock_release(&blocked_list_lock);
	
	if (target->priority < (get_runqueue()->curr_process)->priority){
		(get_runqueue()->curr_process)->ticks_left = 0;
		_trigger_smode_software_interrupt();
	}

	return;
}

void exit_process(unsigned long code){
	spinlock_acquire(&zombie_list_lock);
	spinlock_acquire(&(get_runqueue()->lock));
	if((get_runqueue()->curr_process)->pid == 0) kpanic(113,(get_runqueue()->curr_process)->tf->epc);

	if((get_runqueue()->curr_process)->magic != 0xC00C33E1) kpanic(114,(get_runqueue()->curr_process)->pid);

	if (emergency_process == (get_runqueue()->curr_process)) emergency_process = 0;

	if ((get_runqueue()->curr_process)->next == (get_runqueue()->curr_process)){
		(get_runqueue()->active_process_circles)[(get_runqueue()->curr_process)->priority] = 0;
		(get_runqueue()->lookup_bitmap) &= (~(1UL << (get_runqueue()->curr_process)->priority));
	} else {
		struct process* head = (get_runqueue()->curr_process)->next;
		struct process* tail = (get_runqueue()->curr_process)->prev;

		head->prev = tail;
		tail->next = head;

		if ((get_runqueue()->curr_process) == (get_runqueue()->active_process_circles)[(get_runqueue()->curr_process)->priority]){
			(get_runqueue()->active_process_circles)[(get_runqueue()->curr_process)->priority] = (get_runqueue()->curr_process)->next;
		}
	}

	get_runqueue()->process_count--;
	spinlock_release(&(get_runqueue()->lock));

	(get_runqueue()->curr_process)->next = zombie_process_list_head;
	
	if(zombie_process_list_head != 0) zombie_process_list_head->prev = (get_runqueue()->curr_process);

	(get_runqueue()->curr_process)->prev = 0;

	zombie_process_list_head = (get_runqueue()->curr_process);

	(get_runqueue()->curr_process)->state = 3;
	(get_runqueue()->curr_process)->exit_code = code;
	
	spinlock_release(&zombie_list_lock);
	
	struct process* ptr = 0;

	for (int j = 0; j < hart; j++){
		if (!global_closet[j].rq) continue;
		spinlock_acquire(&(global_closet[j].rq->lock)); 
		for (int i = 0; i < 32; i++){

			if ((get_runqueue()->lookup_bitmap) & (1UL << i)){

				struct process* active_process_list_head = (get_runqueue()->active_process_circles)[i];

				ptr = active_process_list_head;

				do {
					if (ptr->parent_pid == (get_runqueue()->curr_process)->pid){
						ptr->parent_pid = 0;
					}
					ptr = ptr->next;
				} while (ptr != active_process_list_head);
			}
		}
		spinlock_release(&(global_closet[j].rq->lock)); 
	}
	spinlock_acquire(&blocked_list_lock);
	ptr = blocked_process_list_head;

	while (ptr != 0){
		if (ptr->parent_pid == (get_runqueue()->curr_process)->pid){
			ptr->parent_pid = 0;
		}
		ptr = ptr->next;
	}
	spinlock_release(&blocked_list_lock);
	spinlock_acquire(&zombie_list_lock);
	ptr = zombie_process_list_head;

	while (ptr != 0){
		if (ptr->parent_pid == (get_runqueue()->curr_process)->pid){
			ptr->parent_pid = 0;
		}
		ptr = ptr->next;
	}
	spinlock_release(&zombie_list_lock);
	spinlock_acquire(&sleeping_list_lock);
	ptr = sleeping_process_list_head;
	
	while (ptr != 0){
		if (ptr->parent_pid == (get_runqueue()->curr_process)->pid){
			ptr->parent_pid = 0;
		}
		ptr = ptr->next;
	}
	spinlock_release(&sleeping_list_lock);
	spinlock_acquire(&blocked_ipc_list_lock);
	ptr = blocked_ipc_process_list_head;
	
	while (ptr != 0){
		if (ptr->parent_pid == (get_runqueue()->curr_process)->pid){
			ptr->parent_pid = 0;
		}
		ptr = ptr->next;
	}
	spinlock_release(&blocked_ipc_list_lock);
	spinlock_acquire(&blocked_ipc_send_list_lock);
	ptr = blocked_ipc_send_process_list_head;
	
	while (ptr != 0){
		if (ptr->parent_pid == (get_runqueue()->curr_process)->pid){
			ptr->parent_pid = 0;
		}
		ptr = ptr->next;
	}
	spinlock_release(&blocked_ipc_send_list_lock);

	_trigger_smode_software_interrupt();

	return;	
}

unsigned long wait_process(void){
	spinlock_acquire(&zombie_list_lock);
	_off_ssie();

	struct process* header = zombie_process_list_head;

	while (header != 0 && header->parent_pid != (get_runqueue()->curr_process)->pid) header = header->next;

	if (header == 0){
		_set_ssie();
		spinlock_release(&zombie_list_lock);
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

		if (head != 0) head->prev = tail;
		tail->next = head;	
	}
	
	unsigned long code = header->exit_code;
	spinlock_acquire(&alive_counter_lock);
	alive_thread_counter--;
	spinlock_release(&alive_counter_lock);
	unsigned int idx = header->pid / 64;
	unsigned int offset = header->pid % 64;
	spinlock_acquire(&pid_registry_lock);
	pid_registry[idx] &= ~(1UL << offset);
	spinlock_release(&pid_registry_lock);
	uvm_free(header->satp);
	kfree(header->kstack);
	kfree(header->capability_root);
	kvmfree(header);
	
	_set_ssie();
	spinlock_release(&zombie_list_lock);
	return code;
}

void orphan_cleaner(void){
	spinlock_acquire(&zombie_list_lock);

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
			spinlock_acquire(&alive_counter_lock);
			alive_thread_counter--;
			spinlock_release(&alive_counter_lock);
			unsigned int idx = ptr->pid / 64;
			unsigned int offset = ptr->pid % 64;
			spinlock_acquire(&pid_registry_lock);
			pid_registry[idx] &= ~(1UL << offset);
			spinlock_release(&pid_registry_lock);
			
			uvm_free(ptr->satp);
			kfree(ptr->kstack);
			kfree(ptr->capability_root);
			kvmfree(ptr);
		}

		ptr = next_ptr;
	}

	spinlock_release(&zombie_list_lock);
}

void sleep_process (struct process* target, unsigned int ticks){
	spinlock_acquire(&sleeping_list_lock);
	
	if(target->pid == 0) kpanic(113,target->tf->epc);

	if(target->magic != 0xC00C33E1) kpanic(114,target->pid);

	if (emergency_process == target) emergency_process = 0;
	spinlock_acquire(&(get_runqueue()->lock));
	if (target->next == target){
		(get_runqueue()->active_process_circles)[target->priority] = 0;
		(get_runqueue()->lookup_bitmap) &= (~(1UL << target->priority));
	} else {
		struct process* head = target->next;
		struct process* tail = target->prev;

		head->prev = tail;
		tail->next = head;

		if (target == (get_runqueue()->active_process_circles)[target->priority]){
			(get_runqueue()->active_process_circles)[target->priority] = target->next;
		}
	}
	get_runqueue()->process_count--;

	spinlock_release(&(get_runqueue()->lock));
	target->next = sleeping_process_list_head;
	
	if(sleeping_process_list_head != 0) sleeping_process_list_head->prev = target;

	target->prev = 0;

	sleeping_process_list_head = target;

	target->state = 4;

	target->ticks_left = ticks;

	spinlock_release(&sleeping_list_lock);
	
	_trigger_smode_software_interrupt();

	return;
	
}

void block_ipc_process(struct process* target){
	spinlock_acquire(&blocked_ipc_list_lock);

	if(target->pid == 0) kpanic(113,target->tf->epc);

	if(target->magic != 0xC00C33E1) kpanic(114,target->pid);

	if (emergency_process == target) emergency_process = 0;
	spinlock_acquire(&(get_runqueue()->lock));

	if (target->next == target){
		(get_runqueue()->active_process_circles)[target->priority] = 0;
		(get_runqueue()->lookup_bitmap) &= (~(1UL << target->priority));
	} else {
		struct process* head = target->next;
		struct process* tail = target->prev;

		head->prev = tail;
		tail->next = head;

		if (target == (get_runqueue()->active_process_circles)[target->priority]){
			(get_runqueue()->active_process_circles)[target->priority] = target->next;
		}
	}
	get_runqueue()->process_count--;
	spinlock_release(&(get_runqueue()->lock));

	target->next = blocked_ipc_process_list_head;
	
	if(blocked_ipc_process_list_head != 0) blocked_ipc_process_list_head->prev = target;

	target->prev = 0;

	blocked_ipc_process_list_head = target;

	target->state = 5;
	
	spinlock_release(&blocked_ipc_list_lock);
	
	_trigger_smode_software_interrupt();

	return;
}

void unblock_ipc_process(struct process* target){
	spinlock_acquire(&blocked_ipc_list_lock);

	if(target->state != 5) kpanic(115,target->pid);

	if(target->magic != 0xC00C33E1) kpanic(114,target->pid);

	target->state = 0;

	if (target == blocked_ipc_process_list_head){
		blocked_ipc_process_list_head = target->next;

		if (blocked_ipc_process_list_head != 0) blocked_ipc_process_list_head->prev = 0;

	} else {
		struct process* head = target->next;
		struct process* tail = target->prev;

		if (head != 0) head->prev = tail;
		tail->next = head;
	}
	spinlock_acquire(&(get_runqueue()->lock));
	if ((get_runqueue()->lookup_bitmap) & (1UL << target->priority)){
		target->next = (get_runqueue()->active_process_circles)[target->priority];
		struct process* tail = (get_runqueue()->active_process_circles)[target->priority]->prev;
		target->prev = tail;
		tail->next = target;
		(get_runqueue()->active_process_circles)[target->priority]->prev = target;
	} else {
		target->next = target;
		target->prev = target;	
		(get_runqueue()->active_process_circles)[target->priority] = target;
		(get_runqueue()->lookup_bitmap) |= (1UL << target->priority);
	}
	target->tf->kernel_tp = (unsigned long)get_runqueue();
	get_runqueue()->process_count++;
	spinlock_release(&(get_runqueue()->lock));
	spinlock_release(&blocked_ipc_list_lock);

	if (target->priority < (get_runqueue()->curr_process)->priority){
		(get_runqueue()->curr_process)->ticks_left = 0;
		_trigger_smode_software_interrupt();
	}

	return;
}

void block_ipc_send_process(struct process* target){
	spinlock_acquire(&blocked_ipc_send_list_lock);

	if(target->pid == 0) kpanic(113,target->tf->epc);

	if(target->magic != 0xC00C33E1) kpanic(114,target->pid);

	if (emergency_process == target) emergency_process = 0;
	spinlock_acquire(&(get_runqueue()->lock));
	if (target->next == target){
		(get_runqueue()->active_process_circles)[target->priority] = 0;
		(get_runqueue()->lookup_bitmap) &= (~(1UL << target->priority));
	} else {
		struct process* head = target->next;
		struct process* tail = target->prev;

		head->prev = tail;
		tail->next = head;

		if (target == (get_runqueue()->active_process_circles)[target->priority]){
			(get_runqueue()->active_process_circles)[target->priority] = target->next;
		}
	}
	get_runqueue()->process_count--;
	spinlock_release(&(get_runqueue()->lock));
	target->next = blocked_ipc_send_process_list_head;
	
	if(blocked_ipc_send_process_list_head != 0) blocked_ipc_send_process_list_head->prev = target;

	target->prev = 0;

	blocked_ipc_send_process_list_head = target;

	target->state = 6;

	spinlock_release(&blocked_ipc_send_list_lock);

	_trigger_smode_software_interrupt();

	return;
}

void unblock_ipc_send_process(struct process* target){
	spinlock_acquire(&blocked_ipc_send_list_lock);

	if(target->state != 6) kpanic(115,target->pid);

	if(target->magic != 0xC00C33E1) kpanic(114,target->pid);

	target->state = 0;

	if (target == blocked_ipc_send_process_list_head){
		blocked_ipc_send_process_list_head = target->next;

		if (blocked_ipc_send_process_list_head != 0) blocked_ipc_send_process_list_head->prev = 0;

	} else {
		struct process* head = target->next;
		struct process* tail = target->prev;

		if (head != 0) head->prev = tail;
		tail->next = head;
	}
	spinlock_acquire(&(get_runqueue()->lock));
	if ((get_runqueue()->lookup_bitmap) & (1UL << target->priority)){
		target->next = (get_runqueue()->active_process_circles)[target->priority];
		struct process* tail = (get_runqueue()->active_process_circles)[target->priority]->prev;
		target->prev = tail;
		tail->next = target;
		(get_runqueue()->active_process_circles)[target->priority]->prev = target;
	} else {
		target->next = target;
		target->prev = target;	
		(get_runqueue()->active_process_circles)[target->priority] = target;
		(get_runqueue()->lookup_bitmap) |= (1UL << target->priority);
	}
	target->tf->kernel_tp = (unsigned long)get_runqueue();
	get_runqueue()->process_count++;
	spinlock_release(&(get_runqueue()->lock));
	spinlock_release(&blocked_ipc_send_list_lock);

	if (target->priority < (get_runqueue()->curr_process)->priority){
		(get_runqueue()->curr_process)->ticks_left = 0;
		_trigger_smode_software_interrupt();
	}

	return;
}

void update_sleep_list (void){
	spinlock_acquire(&sleeping_list_lock);
	struct process* ptr = sleeping_process_list_head;

	while (ptr != 0) {
		struct process* next_ptr = ptr->next;

		ptr->ticks_left--;

		if (ptr->ticks_left == 0){

			if(ptr->state != 4) kpanic(115,ptr->pid);

			if(ptr->magic != 0xC00C33E1) kpanic(114,ptr->pid);

			ptr->state = 0;

			if (ptr == sleeping_process_list_head){
				sleeping_process_list_head = ptr->next;

				if (sleeping_process_list_head != 0) sleeping_process_list_head->prev = 0;

			} else {
				struct process* head = ptr->next;
				struct process* tail = ptr->prev;

				if (head != 0) head->prev = tail;
				tail->next = head;
			}
			spinlock_acquire(&(get_runqueue()->lock));
			if ((get_runqueue()->lookup_bitmap) & (1UL << ptr->priority)){
				ptr->next = (get_runqueue()->active_process_circles)[ptr->priority];
				struct process* tail = (get_runqueue()->active_process_circles)[ptr->priority]->prev;
				ptr->prev = tail;
				tail->next = ptr;
				(get_runqueue()->active_process_circles)[ptr->priority]->prev = ptr;
			} else {
				ptr->next = ptr;
				ptr->prev = ptr;	
				(get_runqueue()->active_process_circles)[ptr->priority] = ptr;
				(get_runqueue()->lookup_bitmap) |= (1UL << ptr->priority);
			}
			ptr->tf->kernel_tp = (unsigned long)get_runqueue();
			get_runqueue()->process_count++;
			spinlock_release(&(get_runqueue()->lock));
			if (ptr->priority < (get_runqueue()->curr_process)->priority){
				(get_runqueue()->curr_process)->ticks_left = 0;
			}
		}

		ptr = next_ptr;
	}

	spinlock_release(&sleeping_list_lock);
}

void anti_starvation_sweeper(void){
	if (((get_runqueue()->lookup_bitmap) >> 32) == 0) return;

	for (int i = 30; i >= 0; i--){
		
		if ((get_runqueue()->lookup_bitmap) & (1UL << (i + 32))){
			
			if ((get_runqueue()->lookup_bitmap) & (1UL << i)){
				emergency_process = (get_runqueue()->active_process_circles)[i];
				break;
			}
		}

	}

	(get_runqueue()->lookup_bitmap) &= 0xFFFFFFFFUL;
	
	return;
}

int steal_process(void){
	struct process* target = 0;
	for (unsigned long core = 0; core < hart; core++){
		if (!global_closet[core].rq) continue;
		if (global_closet[core].rq == get_runqueue()) continue;
		if ((global_closet[core].rq)->process_count > 1){
			spinlock_acquire(&(global_closet[core].rq->lock));
			for (int i = 0; i < 31; i++){
				if ((global_closet[core].rq)->lookup_bitmap & (1UL << i)){
					struct process* head = global_closet[core].rq->active_process_circles[i];
					struct process* ptr = head;
					int found_target = 0;
					do {
						
						if (ptr != global_closet[core].rq->curr_process){
							target = ptr;
							found_target = 1;
							break;
						}
						ptr = ptr->next;

					} while (ptr != head);

					if (found_target){
						if (target->next == target){
							(global_closet[core].rq->active_process_circles)[target->priority] = 0;
							(global_closet[core].rq->lookup_bitmap) &= (~(1UL << target->priority));
						} else {
							struct process* head = target->next;
							struct process* tail = target->prev;

							head->prev = tail;
							tail->next = head;

							if (target == (global_closet[core].rq->active_process_circles)[target->priority]){
								(global_closet[core].rq->active_process_circles)[target->priority] = target->next;
							}
						}
						global_closet[core].rq->process_count--;
						break;
					}
				}
			}	
			spinlock_release(&(global_closet[core].rq->lock));
			if (target != 0) break;
		}	
	}

	if (target != 0){
		spinlock_acquire(&(get_runqueue()->lock));
		if ((get_runqueue()->lookup_bitmap) & (1UL << target->priority)){
			target->next = (get_runqueue()->active_process_circles)[target->priority];
			struct process* tail = (get_runqueue()->active_process_circles)[target->priority]->prev;
			target->prev = tail;
			tail->next = target;
			(get_runqueue()->active_process_circles)[target->priority]->prev = target;
		} else {
			target->next = target;
			target->prev = target;	
			(get_runqueue()->active_process_circles)[target->priority] = target;
			(get_runqueue()->lookup_bitmap) |= (1UL << target->priority);
		}
		target->tf->kernel_tp = (unsigned long)get_runqueue();
		get_runqueue()->process_count++;
		spinlock_release(&(get_runqueue()->lock));

		return 1;
	}

	return 0;
}

