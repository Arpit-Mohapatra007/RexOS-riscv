#include "memlayout.h"

extern void uart_init(void);
extern void uart_putc(unsigned char text);
extern void uart_puts(char* string);
extern void uart_putsn(char* string, unsigned int max_length);
extern void uart_puth(unsigned long code);
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

void kpanic(unsigned long mcause, unsigned long mepc) {
    	uart_puts("=======================================================\n");
    	uart_puts(" !!! KERNEL PANIC !!!\n");
    	uart_puts("=======================================================\n");
    	uart_puts("\n");
	unsigned long error_code = ( mcause & 0x7FFFFFFFFFFFFFFFUL );
	switch ( ( mcause >> 63 ) & 0x1 ) {
		case 0:
			uart_puts("Exception Caught ! Cause: ");
			uart_puth(mcause);
			switch ( error_code ) {
				case 0:
            	    uart_puts("[Instruction Address Misaligned]");
           			break;
        		case 2: 
        			uart_puts("[Illegal Instruction Fault]");
        			break;
        		case 4: 
        			uart_puts("[Address Misaligned Fault]");
        			break;
        		case 5:
        			uart_puts("[Load Access Fault]");
        			break;
        		case 7:
        			uart_puts("[Store Access Fault]");
        			break;
        		case 8:
        			uart_puts("[Environment Call from U-mode (Syscall Entry !)]");
        			break;
        		case 11:
        			uart_puts("[Environment Call from M-mode]");
        			break;
        		case 12: 
        			uart_puts("[Instruction Page Fault]");
        			break;
        		case 13: 
        			uart_puts("[Load Page Fault]");
        			break;
        		case 15: 
        			uart_puts("[Store Page Fault]");
        			break;
        		default:
        			uart_puts("[Unhandled Hardware Exception]");
        			break;
			}
			break;
		case 1:
			uart_puts("Interrupt Occured ! Cause: ");
			uart_puth(mcause);
			switch ( error_code ) {
				case 1: 
					uart_puts("[Software Interrupt]");
					break;
				case 5: 
					uart_puts("[Timer Interrupt]");
					break;
				case 9: 
					uart_puts("[External Interrupt]");
					break;
				default:
					uart_puts("[Unhandled Hardware Interrrupt]");
					break;
			}
			break;
		default:
			uart_puts("Unknown Fault ! Cause: ");
			uart_puth(mcause);
			uart_puts("[Unknown]");
			break;
	}

	uart_puts("\n");
	uart_puts("Faulting Address: ");
	uart_puth(mepc);
	uart_puts("\n");
	uart_puts("Halting system execution....");
	while(1);
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
