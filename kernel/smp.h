#ifndef SMP_H
#define SMP_H

struct cpu_closet{
	unsigned long* _mmode_stack;
	unsigned long* _smode_stack;
	volatile unsigned long flag;
};

struct hart_runqueue{
	struct process* active_process_circles[32];
	unsigned long lookup_bitmap;
	unsigned long lock;
	struct process* curr_process;
	struct process* idle_process;
	unsigned long starv_ticks;
};

extern struct hart_runqueue* get_runqueue(void);
extern void spinlock_acquire(unsigned long* lock);
extern void spinlock_release(unsigned long* lock);

#endif
