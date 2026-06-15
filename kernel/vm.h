#ifndef VM_H
#define VM_H

void kvm_map(unsigned long* root_table,unsigned long vir_addr,unsigned long phys_addr, unsigned long size, unsigned int perm);
void kvm_init(void);

#endif
