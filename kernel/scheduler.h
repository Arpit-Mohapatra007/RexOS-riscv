#ifndef SCHEDULER_H
#define SCHEDULER_H

struct process{
	unsigned long context[32];
	unsigned long sepc;
	unsigned long sstatus;
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

extern void _load_sscratch(unsigned long curr_process_addr);

void scheduler_init(void);
void round_robin(void);
void spawn_process(void (*entry_function)(void), char* name, unsigned long sstatus_val, unsigned long satp_val);

#endif
