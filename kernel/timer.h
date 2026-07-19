#ifndef TIMER_H
#define TIMER_H

void timer_init(void);
void update_timer(unsigned long mhartid);
void disable_timer_interrupts(unsigned long mhartid);

#endif
