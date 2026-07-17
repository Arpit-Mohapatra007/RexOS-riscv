#include "smp.h"
#include "scheduler.h"
#include "vm.h"
#include "kalloc.h"
#include "timer.h"
#include "uart.h"

extern void _off_ssie(void);
extern void _set_ssie(void);
extern void _set_seie(void);
extern void _wfi(void);
extern void _trigger_smode_software_interrupt(void);
extern void _load_tp(unsigned long hart_runqueue_addr);

extern struct cpu_closet* global_closet;

unsigned long kalloc_lock = 0;
unsigned long kvmalloc_lock = 0;
unsigned long uart_lock = 0;
unsigned long blocked_list_lock = 0;
unsigned long zombie_list_lock = 0;
unsigned long sleeping_list_lock = 0;
unsigned long blocked_ipc_list_lock = 0;
unsigned long blocked_ipc_send_list_lock = 0;
unsigned long pid_registry_lock = 0;
unsigned long alive_counter_lock = 0;

extern char _binary_user_worker_elf_start[];

void idle_loop (void){
	while(1){
		_off_ssie();

		if (steal_process()){
			_set_ssie();
			_set_seie();
			_trigger_smode_software_interrupt();
		} else {
			_set_ssie();
			_set_seie();
			_wfi();
		}
	}
}

void smp_kmain(unsigned long hart_id){
	struct hart_runqueue* core_hart_runqueue = (struct hart_runqueue*)kvmalloc(sizeof(struct hart_runqueue));
	
	unsigned char* ptr = (unsigned char*)core_hart_runqueue;
	
	for (int i = 0; i < sizeof(struct hart_runqueue) ; i++) *(ptr + i) = 0;

	core_hart_runqueue->hart_id = hart_id;
	
	_load_tp((unsigned long)core_hart_runqueue);

	global_closet[hart_id].rq = core_hart_runqueue;

	struct process* idle = (struct process*)kvmalloc(sizeof(struct process));

	ptr = (unsigned char*)idle;

	for (int i = 0; i< sizeof(struct process); i++) *(ptr + i) = 0;

	idle->magic = 0xC00C33E1;
	idle->pid = 0;
	idle->state = 1;
	idle->priority = 31;
	idle->time_slice = ((idle->priority << 2) + 4);
	idle->ticks_left = idle->time_slice;
	idle->tf = 0;
	idle->next = idle;
	idle->prev = idle;

	unsigned long* idle_kstack = (unsigned long*)kalloc(0);
	idle->kstack = idle_kstack;
	idle->context[0] = (unsigned long)idle_loop;
	idle->context[1] = (unsigned long)((char*)idle_kstack + 4096);

	idle->capability_root = (unsigned long*)(kalloc(2));
	for (int i = 0; i < 2048; i++){
		idle->capability_root[i] = 0;
	}

	core_hart_runqueue->idle_process = idle;
	core_hart_runqueue->curr_process = idle;
	core_hart_runqueue->active_process_circles[idle->priority] = idle;
	core_hart_runqueue->lookup_bitmap |= (1UL << idle->priority);
	core_hart_runqueue->process_count = 1;
	
	timer_init();

	_set_ssie();
	_set_seie();

	idle_loop();
}
