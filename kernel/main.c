#include "memlayout.h"
#include "uart.h"
#include "string.h"
#include "kalloc.h"
#include "dtb.h"
#include "vm.h"
#include "timer.h"
#include "scheduler.h"
#include "main.h"

extern void _trigger_smode_software_interrupt(void);
extern void _set_ssie(void);
extern void _set_seie(void);
extern void _off_sip(void);
extern void _off_ssie(void);
extern void _load_satp(unsigned long table_root);
extern void _wfi(void);
extern void _load_umode_stvec(void);
extern void _load_smode_stvec(void);

extern char _binary_user_shell_elf_start[];

extern struct process* curr_process;
extern unsigned long* root_table;

struct process* shell_ptr = 0;

volatile unsigned char uart_buff = 0x00;

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
				case 116:
					uart_puts("[Attempted to Load Corrupted ELF]");
					break;
				case 117:
					uart_puts("[Motherboard does not have a Supervisor PLIC Context]");
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

	    if (curr_process->tf != 0 && (curr_process->tf->sstatus & 0x100) == 0){
			_load_umode_stvec();
	    } else {
			_load_smode_stvec();
	    } 

	} else if ( !((scause >> 63) & 0x1) && error_code == 8){
		curr_process->tf->epc += 4;
		

		unsigned long a7 = curr_process->tf->u_context[17];
		unsigned long a0 = curr_process->tf->u_context[10];

		switch(a7) {
			case 1:	
				uart_putc((unsigned char) a0);
				break;
			case 2:
				shell_ptr = curr_process;
				
				if (uart_buff != 0x00){
					curr_process->tf->u_context[10] = (unsigned long) uart_buff;
					uart_buff = 0x00;
				} else {
					block_process(shell_ptr);
				}

				break;
		}

	} else if (((scause >> 63) & 0x1) && error_code == 9){
		unsigned int irq = plic_claim();
		if (irq == 10){
			unsigned char c = uart_getc();

			if (shell_ptr != 0 && shell_ptr->state == 2) {
				shell_ptr->tf->u_context[10] = (unsigned long) c;
				unblock_process(shell_ptr);
			} else {
				uart_buff = c;
			}
		}

		plic_complete(irq);
	}else {
		kpanic(scause,stval);
	}

	if (curr_process->tf != 0) _load_sscratch((unsigned long)curr_process->tf);

	return;	
}

void oxomoco_loop (void){
	while(1){
		_off_ssie();
		orphan_cleaner();
		_set_ssie();
		_set_seie();
		_wfi();
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
	dtb_parser_context();
	scheduler_init();
	timer_init();
	plic_init();
	uart_ier_enable();
	_set_ssie();
	_set_seie();

	shell_ptr = spawn_process(_binary_user_shell_elf_start, "SHELL", 32);
	
	oxomoco_loop();
}
