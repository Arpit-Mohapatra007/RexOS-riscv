#ifndef UTIL_H
#define UTIL_H

struct ipc_msg{
	unsigned long sender_pid;
	unsigned long type;
	unsigned long data[6];
};

struct process_info{
	unsigned long pid;
	unsigned long parent_pid;
	unsigned int state;
	unsigned int priority;
	unsigned long cpu_time;
	unsigned char name[24];
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
unsigned long grant_permit(unsigned long target_pid, unsigned long permission_pid, unsigned long rights_to_grant);
unsigned long revoke_permit(unsigned long target_pid, unsigned long permission_pid);
unsigned long process_count(void);
unsigned long list_all_process(struct process_info* pif);
unsigned long list_process(struct process_info* pif, unsigned long number_of_process);
unsigned long add_to_whitelist(unsigned long pid);
unsigned long remove_from_whitelist(unsigned long pid);

#endif
