#ifndef SMP_H
#define SMP_H

struct cpu_closet{
	unsigned long* _mmode_stack;
	unsigned long* _smode_stack;
	volatile unsigned long flag;
	struct hart_runqueue* rq;
};

struct hart_runqueue{
	struct process* active_process_circles[32];
	unsigned long lookup_bitmap;
	unsigned long lock;
	struct process* curr_process;
	struct process* idle_process;
	unsigned long starv_ticks;
	unsigned long hart_id;
	unsigned long process_count;
};

extern struct hart_runqueue* get_runqueue(void);
extern void spinlock_acquire(unsigned long* lock);
extern void spinlock_release(unsigned long* lock);

extern unsigned long kalloc_lock;
extern unsigned long kvmalloc_lock;
extern unsigned long uart_lock;
extern unsigned long blocked_list_lock;
extern unsigned long zombie_list_lock;
extern unsigned long sleeping_list_lock;
extern unsigned long blocked_ipc_list_lock;
extern unsigned long blocked_ipc_send_list_lock;
extern unsigned long pid_registry_lock;
extern unsigned long alive_counter_lock;

#endif
