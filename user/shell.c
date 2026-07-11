#include "util.h"

#define CMD_MAX_LEN 64

char cmd_buffer[CMD_MAX_LEN];
unsigned int cmd_idx = 0;

void cmd_parser(char* cmd) {
	if ( strcmp(cmd,"help") ){
		puts("=======================================================\n");
    		puts("HELP MENU\n");
    		puts("=======================================================\n");
		puts("[+] help : Prints Help Menu\n");
		puts("[+] clear : Clears Out Entier Screen\n");
		puts("[+] getpid : Get Process ID of current running process\n");
		puts("[+] recv : Recvieve Message from Mailbox\n");
		puts("[+] ps : Lists all Processes with their details\n");
		puts("=======================================================\n");
	} else if ( strcmp(cmd,"clear") ){
		puts("\033[2J\033[H");
	} else if ( strcmp(cmd, "getpid") ){
		puth(getpid());
		putc('\n');
	} else if ( strcmp(cmd, "recv") ){
		struct ipc_msg buff;
		
		recv_msg(&buff);
		
		puts("SENDER'S PID: ");
		puth(buff.sender_pid);
		putc('\n');
		puts("TYPE OF MESSAGE: ");
		puth(buff.type);
		putc('\n');
		puts("DATA RECIEVED: ");
		puth(buff.data[1]);
		putc('\n');

	} else if ( strcmp(cmd, "ps") ) {
		unsigned long count = process_count();
		puts("\n[+] Total Active Threads: ");
		puth(count);
		putc('\n');

		struct process_info pif[16];
		unsigned long retrieved = list_all_process(pif);

		for (unsigned long i = 0; i < retrieved; i++){
			puts("PID: "); puth(pif[i].pid);
			puts(" | Parent: "); puth(pif[i].parent_pid);
			puts(" | State: "); puth(pif[i].state);
			puts(" | Name: "); puts((char*)pif[i].name);
			putc('\n');
		}

	} else {
		puts("[!] RexOS: command not found: ");
		puts(cmd);
		putc('\n');
	}
	return;
}

void prompt(void){
	puts("RexOS> ");
	return;
}

void shell(void){
	while(1) {
		unsigned char stroke = readc();

		if ( stroke == 0x08 || stroke == 0x7F ){
			if ( cmd_idx > 0 ){
				cmd_idx--;
				putc('\b'); 
				putc(' '); 
				putc('\b');
			}
		} else if ( stroke == '\r'){
				cmd_buffer[cmd_idx] = '\0';
				putc('\r');
				putc('\n' );
				cmd_parser(cmd_buffer);
				cmd_idx = 0;
				prompt();
		} else {
			if ( cmd_idx < (CMD_MAX_LEN - 1) ){
				cmd_buffer[cmd_idx] = stroke;
				putc(stroke);
				cmd_idx++;
			}
		}
	}

}

