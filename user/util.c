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
