#ifndef UTIL_H
#define UTIL_H

struct ipc_msg{
	unsigned long sender_pid;
	unsigned long type;
	unsigned long data[6];
};

void puts(char* string);
void putc(char c);
void puth(unsigned long code); 
int strcmp(char* s1,char* s2);
char readc(void);
unsigned long getpid(void);
void yield(void);
void sleep(unsigned int ticks);
void exit (unsigned long code);
unsigned long wait(void);
unsigned long send_msg(unsigned long dest_pid, struct ipc_msg* msg);
unsigned long recv_msg(struct ipc_msg* buff);

#endif
