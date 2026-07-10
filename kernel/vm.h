#ifndef VM_H
#define VM_H

void kvm_map(unsigned long* root_table,unsigned long vir_addr,unsigned long phys_addr, unsigned long size, unsigned int perm);
void kvm_init(void);
void kvmalloc_init(void);
void* kvmalloc(unsigned long size);
void kvmfree(void* slot);
void uvm_map(unsigned long* user_table,unsigned long vir_addr,unsigned long phys_addr, unsigned long size, unsigned int perm);
unsigned long* create_user_table(unsigned long* process_kstack);
void uvm_free(unsigned long user_satp);
unsigned long uvm_translate(unsigned long user_satp, unsigned long vir_addr);

#endif
