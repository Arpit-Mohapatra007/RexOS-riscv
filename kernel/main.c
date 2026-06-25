#include "memlayout.h"
#include "uart.h"
#include "string.h"
#include "kalloc.h"
#include "dtb.h"
#include "vm.h"
#include "timer.h"
#include "scheduler.h"

extern void _trigger_smode_software_interrupt(void);
extern void _set_ssie(void);
extern void _set_seie(void);
extern void _off_sip(void);
extern void _off_ssie(void);
extern void _load_satp(unsigned long table_root);
extern void _wfi(void);

extern struct process* curr_process;
extern unsigned long* root_table;

#define CMD_MAX_LEN 64

char cmd_buffer[CMD_MAX_LEN];
unsigned int cmd_idx = 0;

struct process* shell_ptr = 0;

void print_banner(void) { 
	uart_puts("\033[38;5;82;48;5;236m");
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

void prompt(void){
	uart_puts("RexOS> ");
	return;
}

void kpanic(unsigned long mcause, unsigned long mepc) {
	uart_puts("\n");
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
				case 1:
					uart_puts("[Instruction Access Fault]");
					break;
        			case 2: 
        				uart_puts("[Illegal Instruction Fault]");
        				break;
				case 3:
					uart_puts("[Breakpoint]");
					break;
        			case 4: 
        				uart_puts("[Load Address Misaligned]");
        				break;
        			case 5:
        				uart_puts("[Load Access Fault]");
        				break;
				case 6:
					uart_puts("[Store Address Misaligned]");
					break;
        			case 7:
        				uart_puts("[Store Access Fault]");
        				break;
        			case 8:
        				uart_puts("[Environment Call from U-mode (Syscall Entry !)]");
        				break;
				case 9:
					uart_puts("[Environment Call from HS-mode]");
					break;
				case 10:
					uart_puts("[Environment Call from VS-mode]");
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
				case 20:
					uart_puts("[Instruction Guest-Page Fault]");
					break;
				case 21:
					uart_puts("[Load Guest-Page Fault]");
					break;
				case 22:
					uart_puts("[Virtual Instruction]");
					break;
				case 23:
					uart_puts("[Store Guest-Page Fault]");
					break;
				case 100:
					uart_puts("[Requested Block Order Exceeds MAX_ORDER Limit]");
					break;
				case 101:
					uart_puts("[Out of Physical Memory]");
					break;
				case 102:
					uart_puts("[Attempted to Free an OUT-OF-BOUNDS Physical Address]");
					break;
				case 103:
					uart_puts("[Double-Free or Corruption Detected]");
					break;
				case 104:
					uart_puts("[Physical Address is Misaligned]");
					break;
				case 105:
					uart_puts("Attempted to Free an Invalid Block Head or Interior Page");
					break;
				case 106:
					uart_puts("[Metadata Size Suffocates Available Physical RAM]");
					break;
				case 107:
					uart_puts("[Memory Protection Violation]");
					break;
				case 108:
					uart_puts("[Out of Physical Memory for New Slab]");
					break;
				case 109:
					uart_puts("[Out of Memory for Off-Slab Descriptor Headers]");
					break;
				case 110:
					uart_puts("[Failed to Allocate Level-1 Page Table]");
					break;
				case 111:
					uart_puts("[Failed to Allocate Level-0 Page Table]");
					break;
				case 112:
					uart_puts("[Cannot Allocate Root Page Table]");
					break;
				case 113:
					uart_puts("[Attempted to Block Idle Thread]");
					break;
				case 114:
					uart_puts("[Corrupted Thread]");
					break;
				case 115:
					uart_puts("[Attempted to Unblock an Active Thread]");
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
					uart_puts("[Supervisor Software Interrupt]");
					break;
				case 2:
					uart_puts("[Virtual Supervisor Software Interrupt]");
					break;
				case 3:
					uart_puts("[Machine Software Interrupt]");
					break;
				case 5: 
					uart_puts("[Supervisor Timer Interrupt]");
					break;
				case 6:
					uart_puts("[Virtual Supervisor Timer Interrupt]");
					break;
				case 7:
					uart_puts("[Machine Time Interrupt]");
					break;
				case 9: 
					uart_puts("[Supervisor External Interrupt]");
					break;
				case 10:
					uart_puts("[Virtual Supervisor External Interrupt]");
					break;
				case 11:
					uart_puts("[Machine External Interrupt]");
					break;
				case 12:
					uart_puts("[Supervisor Guest External Interrupt]");
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

void cmd_parser(char* cmd) {
	if ( kstrcmp(cmd,"help") ){
		uart_puts("=======================================================\n");
    		uart_puts("HELP MENU\n");
    		uart_puts("=======================================================\n");
		uart_puts("[+] help : Prints Help Menu\n");
		uart_puts("[+] clear : Clears Out Entier Screen\n");
		uart_puts("=======================================================\n");
	} else if ( kstrcmp(cmd,"clear") ){
		uart_puts("\033[2J\033[H");
	} else {
		uart_puts("[!] RexOS: command not found: ");
		uart_puts(cmd);
		uart_putc('\n');
	}
	return;
}

void mtrap_router(unsigned long mcause, unsigned long mepc){
	unsigned long error_code = ( mcause & 0x7FFFFFFFFFFFFFFFUL );

	if (((mcause >> 63) & 0x1) && error_code == 7){
		update_timer();
		_trigger_smode_software_interrupt();
	} else {
		kpanic(mcause,mepc);
	}		
	return;
}

void strap_router(unsigned long scause, unsigned long stval){
	unsigned long error_code = ( scause & 0x7FFFFFFFFFFFFFFFUL );

	if (((scause >> 63) & 0x1) && error_code == 1){
		_off_sip();
		round_robin();
		_load_satp(curr_process->satp);
		_load_sscratch((unsigned long)curr_process);
	} else if (((scause >> 63) & 0x1) && error_code == 9){
		unsigned int irq = plic_claim();

		if (irq == 10){
			if (shell_ptr->state == 2) unblock_process(shell_ptr);
		}

		plic_complete(irq);
	}else {
		kpanic(scause,stval);
	}		
	return;	
}

void shell(void){
	while(1) {
		unsigned char stroke = uart_getc();
		if (stroke == 0x00) {
			block_process(curr_process);
			continue;
		}

		if ( stroke == 0x08 || stroke == 0x7F ){
			if ( cmd_idx > 0 ){
				cmd_idx--;
				uart_putc('\b'); 
				uart_putc(' '); 
				uart_putc('\b');
			}
		} else if ( stroke == '\r'){
				cmd_buffer[cmd_idx] = '\0';
				uart_putc('\r');
				uart_putc('\n' );
				cmd_parser(cmd_buffer);
				cmd_idx = 0;
				prompt();
		} else {
			if ( cmd_idx < (CMD_MAX_LEN - 1) ){
				cmd_buffer[cmd_idx] = stroke;
				uart_putc(stroke);
				cmd_idx++;
			}
		}
	}

}

void worker_alpha(void){
		uart_puts("[A]");

		for (long i = 0; i < 1000000000; i++);

		uart_puts("[A-DIES]");

		exit_process(0);

		uart_puts("[FORBIDDEN]");
		while(1);
}

void worker_beta(void){
	while(1){
		uart_puts("[B]");

		for (long i = 0; i < 1000000000; i++);
	}
}

void kmain(void) {
	uart_init();
	print_banner();
	prompt();
	kalloc_init();
	kvmalloc_init();
	kvm_init();
	dtb_parser_time();
	scheduler_init();
	timer_init();
	struct process* A = spawn_process(worker_alpha,"TEST1",0x00000120,(unsigned long)root_table);
	struct process* B = spawn_process(worker_beta,"TEST2",0x00000120,(unsigned long)root_table);
	shell_ptr = spawn_process(shell,"SHELL",0x00000120,(unsigned long)root_table);
	plic_init();
	uart_ier_enable();
	_set_ssie();
	_set_seie();
	while(1){
		_off_ssie();
		orphan_cleaner();
		_set_ssie();
		_wfi();
	}
}
