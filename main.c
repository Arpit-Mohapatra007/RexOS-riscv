#include "memlayout.h"

extern void uart_init(void);
extern void uart_putc(unsigned char text);
extern void uart_puts(char* string);
extern void uart_putsn(char* string, unsigned int max_length);
extern unsigned char uart_getc(void);

void print_banner(void) {
	uart_puts("\033[2J\033[H"); 
    	uart_puts(" ______               ____   _____\n");
    	uart_puts(" | ___ \\             / __ \\ /  ___|\n");
    	uart_puts(" | |_/ /___ _  _    | |  | |\\ `--. \n");
    	uart_puts(" |    // _ \\ \\/ /   | |  | | `--. \\\n");
    	uart_puts(" | |\\ \\  __/>  <    | |__| |/\\__/ /\n");
    	uart_puts(" \\_| \\_\\___/_/\\_\\    \\____/ \\____/ \n");
    	uart_puts("\n");
    	uart_puts("=======================================================\n");
    	uart_puts(" Native 64-bit RISC-V Microkernel - By DreadX\n");
    	uart_puts("=======================================================\n");
    	uart_puts("\n");
	return;
}

void kmain(void) {
	uart_init();
	print_banner();
	while(1) {
		unsigned char stroke = uart_getc();
		if(stroke == 0x08 || stroke == 0x7F){
			uart_putc('\b'); uart_putc(' '); uart_putc('\b');
		} else {
			uart_putc(stroke);
			if (stroke == '\r') uart_putc('\n');
		}
	}
}
