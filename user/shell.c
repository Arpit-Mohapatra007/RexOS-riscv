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
		puts("=======================================================\n");
	} else if ( strcmp(cmd,"clear") ){
		puts("\033[2J\033[H");
	} else if ( strcmp(cmd, "getpid") ){
		puth(getpid());
		putc('\n');
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

