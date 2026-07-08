#ifndef SCHEDULER_H
#define SCHEDULER_H

struct process{
	unsigned long context[14];
	struct trapframe* tf;
	unsigned long domain_id;
	unsigned long satp;
	unsigned long* kstack;
	unsigned long pid;
	unsigned long parent_pid;
	unsigned char name[24];
	unsigned int state;
	unsigned int priority;
	unsigned int time_slice;
	unsigned int ticks_left;
	unsigned long cpu_time;
	unsigned long* capability_root;
	unsigned long syscall_bitmap;
	unsigned long stack_canary;
	unsigned long syscall_ring[8];
	unsigned long ring_idx;
	unsigned long page_faults;
	unsigned long* mmap_root;
	unsigned long fds_root;
	unsigned long signals;
	struct process* next;
	struct process* prev;
	unsigned long exit_code;
	unsigned long magic;
};

struct trapframe{
	unsigned long u_context[32];
	unsigned long epc;
	unsigned long sstatus;
	unsigned long kernel_satp;
	unsigned long kernel_sp;
	unsigned long kernel_trap;
	unsigned long user_satp;
};

extern void _load_sscratch(unsigned long curr_process_addr);

void scheduler_init(void);
void round_robin(void);
struct process* spawn_process(char* _binary, char* name, unsigned long sstatus_val);
void block_process(struct process* target);
void unblock_process(struct process* target);
void exit_process(unsigned long code);
unsigned long wait_process(void);
void orphan_cleaner(void);

#endif
