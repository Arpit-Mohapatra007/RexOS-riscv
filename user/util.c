#include "util.h"

extern void _sys_write(unsigned char c);
extern char _sys_read(void);
extern unsigned long _sys_getpid(void);
extern void _sys_yield(void);
extern void _sys_sleep(unsigned int ticks);
extern void _sys_exit(unsigned long code);
extern unsigned long _sys_wait(void);
extern unsigned long _sys_send_msg(unsigned long dest_pid, struct ipc_msg* msg);
extern unsigned long _sys_recv_msg(struct ipc_msg* buff);
extern unsigned long _sys_grant_permit(unsigned long target_pid, unsigned long permission_pid, unsigned long rights_to_grant);
extern unsigned long _sys_revoke_permit(unsigned long target_pid, unsigned long permission_pid);
extern unsigned long _sys_process_count(void);
extern unsigned long _sys_process_list(struct process_info* pif, unsigned long num_of_process);
extern unsigned long _sys_update_whitelist(unsigned long pid, unsigned long mode);
extern unsigned long _sys_map_shared_page(unsigned long target_pid, unsigned long caller_vaddr, unsigned long target_vaddr, unsigned long permission);
extern unsigned long _sys_unmap_shared_page(unsigned long target_pid, unsigned long target_vaddr);
extern unsigned long _sys_register_irq(unsigned long irq_number);
extern unsigned long _sys_probe_vaddr(unsigned long vaddr);

void puts(char* string){
	unsigned long i = 0;
	while (string[i] != '\0'){
		putc(string[i]);
		i++;
	}
	return;
}

void puth(unsigned long code) {
	for (int i = 0; i < 16; i++) {
		int nibble = ( ( code >> ( 60 - ( i * 4 ) ) ) & 0xF );
		if ( nibble <= 9 ) {
			putc((char)(nibble + '0'));
		} else {
			putc((char)(nibble - 10 + 'A'));
		}
	}
	return;
}

void putc(char c){
	_sys_write(c);
	return;
}

int strcmp(char* s1,char* s2) {
	int idx = 0;
	while(1){
		if ( s1[idx] != s2[idx] ) return 0;
		if ( s1[idx] == '\0' ) return 1;
		idx++;
	}
}

char readc(void){
	return _sys_read();
}

unsigned long getpid(void){
	return _sys_getpid();
}

void yield(void){
	return _sys_yield();
}

void sleep(unsigned int ticks){
	_sys_sleep(ticks);
	return;
}

void exit (unsigned long code){
	_sys_exit(code);
	return;
}

unsigned long wait(void){
	return _sys_wait();
}

unsigned long send_msg(unsigned long dest_pid, struct ipc_msg* msg){
	return _sys_send_msg(dest_pid,msg);
}

unsigned long recv_msg(struct ipc_msg* buff){
	return _sys_recv_msg(buff);
}

unsigned long grant_permit(unsigned long target_pid, unsigned long permission_pid, unsigned long rights_to_grant){
	return _sys_grant_permit(target_pid,permission_pid,rights_to_grant);
}

unsigned long revoke_permit(unsigned long target_pid, unsigned long permission_pid){
	return _sys_revoke_permit(target_pid,permission_pid);
}

unsigned long process_count(void){
	return _sys_process_count();
}

unsigned long list_all_process(struct process_info* pif){
	unsigned long total_process = process_count();
	return _sys_process_list(pif, total_process);
}

unsigned long list_process(struct process_info* pif, unsigned long number_of_process){
	return _sys_process_list(pif, number_of_process);
}

unsigned long add_to_whitelist(unsigned long pid){
	return _sys_update_whitelist(pid, (unsigned long)1);
}

unsigned long remove_from_whitelist(unsigned long pid){
	return _sys_update_whitelist(pid, (unsigned long)0);
}

unsigned long map_shared_page(unsigned long target_pid, unsigned long caller_vaddr, unsigned long target_vaddr, unsigned long permission){
	return _sys_map_shared_page(target_pid, caller_vaddr, target_vaddr, permission);
}
unsigned long unmap_shared_page(unsigned long target_pid, unsigned long target_vaddr){
	return _sys_unmap_shared_page(target_pid, target_vaddr);
}

unsigned long register_irq(unsigned long irq_number){
	return _sys_register_irq(irq_number);
}

unsigned long probe_vaddr(unsigned long vaddr){
	return _sys_probe_vaddr(vaddr);
}
