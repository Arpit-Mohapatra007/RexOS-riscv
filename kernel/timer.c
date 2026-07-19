#include "memlayout.h"
#include "dtb.h"
#include "smp.h"

extern void _set_ssie(void);
extern void _enable_timer(void);

void timer_init(void){
	unsigned long time_lapse = timebase_freq / 100;
	unsigned long curr_time = *(volatile unsigned long*)(clint.base_addr + 0xBFF8);
	*(volatile unsigned long*)(clint.base_addr + 0x4000 + ((unsigned long)(get_runqueue()->hart_id) * 8)) = curr_time + time_lapse;
	_enable_timer();
	_set_ssie();
	return;
}

void update_timer(unsigned long mhartid){
	unsigned long time_lapse = timebase_freq / 100;
	unsigned long curr_time = *(volatile unsigned long*)(clint.base_addr + 0xBFF8);
	*(volatile unsigned long*)(clint.base_addr + 0x4000 + (mhartid * 8)) = curr_time + time_lapse;
	return;	
}

void disable_timer_interrupts(unsigned long mhartid){
	unsigned long curr_time = *(volatile unsigned long*)(clint.base_addr + 0xBFF8);
	*(volatile unsigned long*)(clint.base_addr + 0x4000 + (mhartid * 8)) = -1;
	return;		
}
