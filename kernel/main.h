#ifndef MAIN_H
#define MAIN_H

void kpanic(unsigned long mcause, unsigned long mepc);
void strap_router(unsigned long scause, unsigned long stval);
void oxomoco_loop (void);

#endif
