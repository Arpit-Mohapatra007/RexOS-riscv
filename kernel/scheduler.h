#ifndef SCHEDULER_H
#define SCHEDULER_H

struct ipc_msg{
	unsigned long sender_pid;
	unsigned long type;
	unsigned long data[7];
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

struct process_info{
	unsigned long pid;
	unsigned long parent_pid;
	unsigned int state;
	unsigned int priority;
	unsigned long cpu_time;
	unsigned char name[24];
};

struct process{
	unsigned long context[14];
	struct trapframe* tf;
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
	struct ipc_msg mailbox;
	unsigned long ipc_pending_pid;
	unsigned int mailbox_status;
	unsigned int active_grant;
	unsigned long grant_whitelist[8];
	struct process* next;
	struct process* prev;
	unsigned long exit_code;
	unsigned long magic;
};

extern void _load_sscratch(unsigned long curr_process_addr);

extern struct process* curr_process;
extern struct process* active_process_circles[32];
extern unsigned long lookup_bitmap;
extern struct process* blocked_process_list_head;
extern struct process* zombie_process_list_head;
extern struct process* sleeping_process_list_head;
extern struct process* blocked_ipc_process_list_head;
extern struct process* blocked_ipc_send_process_list_head;
extern struct process* idle_process;

extern unsigned long alive_thread_counter;
extern unsigned long* pid_registry;

void scheduler_init(void);
void schedule(void);
struct process* spawn_process(char* _binary, char* name, unsigned long sstatus_val, unsigned int priority);
void block_process(struct process* target);
void unblock_process(struct process* target);
void exit_process(unsigned long code);
unsigned long wait_process(void);
void orphan_cleaner(void);
void sleep_process (struct process* target, unsigned int ticks);
void block_ipc_process(struct process* target);
void unblock_ipc_process(struct process* target);
void block_ipc_send_process(struct process* target);
void unblock_ipc_send_process(struct process* target);
void update_sleep_list (void);
void anti_starvation_sweeper(void);

#endif
