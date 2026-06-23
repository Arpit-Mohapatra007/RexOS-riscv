#include "memlayout.h"
#include "dtb.h"

extern void _set_ssie(void);

void timer_init(void){
	unsigned long time_lapse = timebase_freq / 100;
	unsigned long curr_time = *(volatile unsigned long*)(CLINT_BASE + 0xBFF8);
	*(volatile unsigned long*)(CLINT_BASE + 0x4000) = curr_time + time_lapse;
	_set_ssie();
	return;
}

void update_timer(void){
	unsigned long time_lapse = timebase_freq / 100;
	unsigned long curr_time = *(volatile unsigned long*)(CLINT_BASE + 0xBFF8);
	*(volatile unsigned long*)(CLINT_BASE + 0x4000) = curr_time + time_lapse;
	return;	
}
