#include "smp.h"
#include "scheduler.h"
#include "vm.h"

extern void _set_ssie(void);
extern void _set_seie(void);
extern void _wfi(void);
extern void _load_tp(unsigned long hart_runqueue_addr);

void idle_loop (void){
	while(1){
		_wfi();
	}
}

void smp_kmain(void){
	struct hart_runqueue* core_hart_runqueue = (struct hart_runqueue*)kvmalloc(sizeof(struct hart_runqueue));
	
	unsigned char* ptr = (unsigned char*)core_hart_runqueue;
	
	for (int i = 0; i < sizeof(struct hart_runqueue) ; i++){
		*(ptr + i) = 0;
	}
	
	_load_tp((unsigned long)core_hart_runqueue);

	struct process* idle = spawn_process((char *)idle_loop, "IDLE", 32, 31);

	core_hart_runqueue->idle_process = idle;

	_set_ssie();
	_set_seie();
	
	idle_loop();
}
