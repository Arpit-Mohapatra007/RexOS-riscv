#include "memlayout.h"
#include "uart.h"
#include "string.h"
#include "kalloc.h"
#include "dtb.h"
#include "vm.h"
#include "timer.h"
#include "scheduler.h"
#include "main.h"
#include "smp.h"

extern void _trigger_smode_software_interrupt(void);
extern void _set_ssie(void);
extern void _set_seie(void);
extern void _off_sip(void);
extern void _off_ssie(void);
extern void _load_satp(unsigned long table_root);
extern void _wfi(void);
extern void _load_umode_stvec(void);
extern void _load_smode_stvec(void);
extern void _flush_tlb(void);
extern void _load_tp(unsigned long hart_runqueue_addr);

extern char _binary_user_shell_elf_start[];
extern char _binary_user_worker_elf_start[];

extern unsigned long* root_table;
struct cpu_closet* global_closet;

struct process* shell_ptr = 0;
unsigned long irq_registry[64] = {0};
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

		(get_runqueue()->curr_process)->cpu_time++;
		
		if ((get_runqueue()->curr_process)->ticks_left > 0) (get_runqueue()->curr_process)->ticks_left--;

		update_sleep_list();
		(get_runqueue()->starv_ticks)++;
		
		if ((get_runqueue()->starv_ticks) == 1000){
			anti_starvation_sweeper();
			(get_runqueue()->starv_ticks) = 0;
		}

		if ((get_runqueue()->curr_process)->ticks_left == 0 || (get_runqueue()->curr_process)->state != 1){
			schedule();
		}

	    if ((get_runqueue()->curr_process)->tf != 0 && ((get_runqueue()->curr_process)->tf->sstatus & 0x100) == 0){
			_load_umode_stvec();
	    } else {
			_load_smode_stvec();
	    } 

	} else if ( !((scause >> 63) & 0x1) && error_code == 8){
		(get_runqueue()->curr_process)->tf->epc += 4;
		

		unsigned long a7 = (get_runqueue()->curr_process)->tf->u_context[17];
		unsigned long a0 = (get_runqueue()->curr_process)->tf->u_context[10];
		unsigned long a1 = (get_runqueue()->curr_process)->tf->u_context[11];
		unsigned long a2 = (get_runqueue()->curr_process)->tf->u_context[12];
		unsigned long a3 = (get_runqueue()->curr_process)->tf->u_context[13];
		unsigned long a4 = (get_runqueue()->curr_process)->tf->u_context[14];
		unsigned long a5 = (get_runqueue()->curr_process)->tf->u_context[15];
		unsigned long a6 = (get_runqueue()->curr_process)->tf->u_context[16];

		switch(a7) {
			case 1:	
				uart_putc((unsigned char) a0);
				break;
			case 2:
				shell_ptr = (get_runqueue()->curr_process);
				
				if (uart_buff != 0x00){
					(get_runqueue()->curr_process)->tf->u_context[10] = (unsigned long) uart_buff;
					uart_buff = 0x00;
				} else {
					block_process(shell_ptr);
				}

				break;
			case 3:
				exit_process(a0);
				break;
			case 4:
				(get_runqueue()->curr_process)->tf->u_context[10] = (get_runqueue()->curr_process)->pid;
				break;
			case 5:
				(get_runqueue()->curr_process)->state = 0;
				_trigger_smode_software_interrupt();
				break;
			case 6:
				sleep_process((get_runqueue()->curr_process), (unsigned int)a0);
				break;
			case 7:
				(get_runqueue()->curr_process)->tf->u_context[10] = wait_process();
				break;
			case 8: {
					unsigned long phys_addr = uvm_translate((get_runqueue()->curr_process)->satp, a1);

					if (phys_addr == 0){
						uart_puts("\n [RexOS] Process ");
						uart_puth((get_runqueue()->curr_process)->pid);
						uart_puts(" [");
						uart_puts((char*)(get_runqueue()->curr_process)->name);
						uart_puts("] ");
						uart_puts(" terminated (Segmentation Fault - Bad Syscall Pointer)\n");
						exit_process(-1);
						break;
					}

					struct process* target = 0;
					struct process* ptr = 0;	
					
					for (int i = 0; i < 32; i++){

						if ((get_runqueue()->lookup_bitmap) & (1UL << i)){
							struct process* active_process_list_head = (get_runqueue()->active_process_circles)[i];
							ptr = active_process_list_head;

							do {
								if ( ptr->pid == a0 ){
									target = ptr;
								}
			
								ptr = ptr->next;
							} while (ptr != active_process_list_head && target == 0);
						}
					}

					ptr = blocked_process_list_head;

					while (ptr != 0 && target == 0){
						if ( ptr->pid == a0){
							target = ptr;
						}

						ptr = ptr->next;
					}

					ptr = blocked_ipc_process_list_head;

					while (ptr != 0 && target == 0){
						if ( ptr->pid == a0){
							target = ptr;
						}

						ptr = ptr->next;
					}

					ptr = blocked_ipc_send_process_list_head;

					while (ptr != 0 && target == 0){
						if ( ptr->pid == a0){
							target = ptr;
						}

						ptr = ptr->next;
					}
					
					ptr = sleeping_process_list_head;

					while (ptr != 0 && target == 0){
						if ( ptr->pid == a0){
							target = ptr;
						}

						ptr = ptr->next;
					}
					
					if (target != 0){
						
						unsigned int idx = target->pid / 16;
						unsigned int offset = ((target->pid % 16) * 4);

						if (!((get_runqueue()->curr_process)->capability_root[idx] & (15UL << offset))) {
							uart_puts("\n [RexOS] Process ");
							uart_puth((get_runqueue()->curr_process)->pid);
							uart_puts(" [");
							uart_puts((char*)(get_runqueue()->curr_process)->name);
							uart_puts("] ");
							uart_puts("(Access Denied to send message to - ");
							uart_puth(target->pid);
							uart_puts(" [");
							uart_puts((char*)target->name);
							uart_puts("] ");
							uart_puts(")");
							uart_putc('\n');

							(get_runqueue()->curr_process)->tf->u_context[10] = (unsigned long)-1;
							break;
						}
						
						if (!((get_runqueue()->curr_process)->capability_root[idx] & (2UL << offset))) {
							uart_puts("\n [RexOS] Process ");
							uart_puth((get_runqueue()->curr_process)->pid);
							uart_puts(" [");
							uart_puts((char*)(get_runqueue()->curr_process)->name);
							uart_puts("] ");
							uart_puts("(Write Rights Denied to - ");
							uart_puth(target->pid);
							uart_puts(" [");
							uart_puts((char*)target->name);
							uart_puts("] ");
							uart_puts(")");
							uart_putc('\n');

							(get_runqueue()->curr_process)->tf->u_context[10] = (unsigned long)-1;
							break;
						}

						idx = (get_runqueue()->curr_process)->pid / 16;
						offset = (((get_runqueue()->curr_process)->pid % 16)* 4);

						if (!(target->capability_root[idx] & (1UL << offset))) {
							uart_puts("\n [RexOS] Process ");
							uart_puth(target->pid);
							uart_puts(" [");
							uart_puts((char*)target->name);
							uart_puts("] ");
							uart_puts("(Read Rights Denied to - ");
							uart_puth((get_runqueue()->curr_process)->pid);
							uart_puts(" [");
							uart_puts((char*)(get_runqueue()->curr_process)->name);
							uart_puts("] ");
							uart_puts(")");
							uart_putc('\n');

							(get_runqueue()->curr_process)->tf->u_context[10] = (unsigned long)-1;
							break;
						}


						if (target->mailbox_status == 0) {
							for (int i = 0; i < (sizeof(struct ipc_msg) / 8); i++){
								*(((unsigned long*)(&target->mailbox)) + i) = *(((unsigned long*)phys_addr) + i);
							}

							target->mailbox.sender_pid = (get_runqueue()->curr_process)->pid;

							if (target->state == 5) unblock_ipc_process(target);

							target->mailbox_status = 1;

							(get_runqueue()->curr_process)->tf->u_context[10] = 1;

						} else {
							(get_runqueue()->curr_process)->tf->epc -= 4;
							(get_runqueue()->curr_process)->ipc_pending_pid = target->pid;
							block_ipc_send_process((get_runqueue()->curr_process));	
						}
					} else {
						(get_runqueue()->curr_process)->tf->u_context[10] = (unsigned long)-1;
					}

					break;
				}
			case 9: {
					unsigned long phys_addr = uvm_translate((get_runqueue()->curr_process)->satp, a0);

					if (phys_addr == 0){
						uart_puts("\n [RexOS] Process ");
						uart_puth((get_runqueue()->curr_process)->pid);
						uart_puts(" [");
						uart_puts((char*)(get_runqueue()->curr_process)->name);
						uart_puts("] ");
						uart_puts(" terminated (Segmentation Fault - Bad Syscall Pointer)\n");
						exit_process(-1);
						break;
					}
				
					if ((get_runqueue()->curr_process)->mailbox_status == 0){
						block_ipc_process((get_runqueue()->curr_process));
					} else {
						for (int i = 0; i < (sizeof(struct ipc_msg) / 8); i++){
							*(((unsigned long*)phys_addr) + i) = *(((unsigned long*)(&(get_runqueue()->curr_process)->mailbox)) + i);
						}
					
						(get_runqueue()->curr_process)->mailbox_status = 0;
						(get_runqueue()->curr_process)->tf->u_context[10] = 1;
						
						struct process* ptr = blocked_ipc_send_process_list_head;
						while (ptr != 0){
							struct process* next_ptr = ptr->next;

							if (ptr->ipc_pending_pid == (get_runqueue()->curr_process)->pid) unblock_ipc_send_process(ptr);

							ptr = next_ptr;
						}
					}

					break;
				}
			case 10:{
					struct process* target = 0;

					struct process* ptr = 0;
					
					for (int i = 0; i < 32; i++){

						if ((get_runqueue()->lookup_bitmap) & (1UL << i)){
							struct process* active_process_list_head = (get_runqueue()->active_process_circles)[i];
							ptr = active_process_list_head;

							do {
								if ( ptr->pid == a0 ){
									target = ptr;
								}
			
								ptr = ptr->next;
							} while (ptr != active_process_list_head && target == 0);
						}
					}

					ptr = blocked_process_list_head;

					while (ptr != 0 && target == 0){
						if ( ptr->pid == a0){
							target = ptr;
						}

						ptr = ptr->next;
					}

					ptr = blocked_ipc_process_list_head;

					while (ptr != 0 && target == 0){
						if ( ptr->pid == a0){
							target = ptr;
						}

						ptr = ptr->next;
					}

					ptr = blocked_ipc_send_process_list_head;

					while (ptr != 0 && target == 0){
						if ( ptr->pid == a0){
							target = ptr;
						}

						ptr = ptr->next;
					}
					
					ptr = sleeping_process_list_head;

					while (ptr != 0 && target == 0){
						if ( ptr->pid == a0){
							target = ptr;
						}

						ptr = ptr->next;
					}
					
					if (target != 0){
						unsigned int idx = a1 / 16;
						unsigned int offset = ((a1 % 16) * 4);

						unsigned long rights = (((get_runqueue()->curr_process)->capability_root[idx] >> offset) & 15UL );

						if ( !(rights & 4UL) ) {
							uart_puts("\n [RexOS] Process ");
							uart_puth((get_runqueue()->curr_process)->pid);
							uart_puts(" [");
							uart_puts((char*)(get_runqueue()->curr_process)->name);
							uart_puts("] ");
							uart_puts("(Don't have right to grant )");
							uart_putc('\n');

							(get_runqueue()->curr_process)->tf->u_context[10] = (unsigned long)-1;
							break;
						}

						if ((rights & (a2 & 15UL)) != (a2 & 15UL)){
							uart_puts("\n [RexOS] Process ");
							uart_puth((get_runqueue()->curr_process)->pid);
							uart_puts(" [");
							uart_puts((char*)(get_runqueue()->curr_process)->name);
							uart_puts("] ");
							uart_puts("(Can't grant unpossessed rights )");
							uart_putc('\n');

							(get_runqueue()->curr_process)->tf->u_context[10] = (unsigned long)-1;
							break;
	
						}

						if ((rights & 8UL) || (get_runqueue()->curr_process)->pid == 1) {
						
							target->capability_root[idx] = (target->capability_root[idx] & (~(15UL << offset))) | ((a2 & 15UL ) << offset);
							(get_runqueue()->curr_process)->tf->u_context[10] = (unsigned long)1;
							break;
						}

						int auth = 0;

						for (int i = 0; i < (get_runqueue()->curr_process)->active_grant; i++){

							if ((get_runqueue()->curr_process)->grant_whitelist[i] == a0){
								auth = 1;
								break;
							}

						}

						if (auth){
							target->capability_root[idx] = (target->capability_root[idx] & (~(15UL << offset))) | ((a2 & 15UL ) << offset);
							(get_runqueue()->curr_process)->tf->u_context[10] = (unsigned long)1;
						} else {
							uart_puts("\n [RexOS] Process ");
							uart_puth((get_runqueue()->curr_process)->pid);
							uart_puts(" [");
							uart_puts((char*)(get_runqueue()->curr_process)->name);
							uart_puts("] ");
							uart_puts("(This PID is not in whitelist : ");
							uart_puth(a0);
							uart_puts(")");
							uart_putc('\n');

							(get_runqueue()->curr_process)->tf->u_context[10] = (unsigned long)-1;
						}
							break;
	
					} else {
						(get_runqueue()->curr_process)->tf->u_context[10] = (unsigned long)-1;
					}

					break;
				}

			case 11:{
					struct process* target = 0;

					struct process* ptr = 0;
					
					for (int i = 0; i < 32; i++){

						if ((get_runqueue()->lookup_bitmap) & (1UL << i)){
							struct process* active_process_list_head = (get_runqueue()->active_process_circles)[i];
							ptr = active_process_list_head;

							do {
								if ( ptr->pid == a0 ){
									target = ptr;
								}
			
								ptr = ptr->next;
							} while (ptr != active_process_list_head && target == 0);
						}
					}

					ptr = blocked_process_list_head;

					while (ptr != 0 && target == 0){
						if ( ptr->pid == a0){
							target = ptr;
						}

						ptr = ptr->next;
					}

					ptr = blocked_ipc_process_list_head;

					while (ptr != 0 && target == 0){
						if ( ptr->pid == a0){
							target = ptr;
						}

						ptr = ptr->next;
					}

					ptr = blocked_ipc_send_process_list_head;

					while (ptr != 0 && target == 0){
						if ( ptr->pid == a0){
							target = ptr;
						}

						ptr = ptr->next;
					}
					
					ptr = sleeping_process_list_head;

					while (ptr != 0 && target == 0){
						if ( ptr->pid == a0){
							target = ptr;
						}

						ptr = ptr->next;
					}
					
					if (target != 0){
						unsigned int idx = a1 / 16;
						unsigned int offset = ((a1 % 16) * 4);
						
						if (target->parent_pid != (get_runqueue()->curr_process)->pid && (get_runqueue()->curr_process)->pid != 1){
							uart_puts("\n [RexOS] Process ");
							uart_puth((get_runqueue()->curr_process)->pid);
							uart_puts(" [");
							uart_puts((char*)(get_runqueue()->curr_process)->name);
							uart_puts("] ");
							uart_puts("(Permission denied to revoke permission as PID: ");
							uart_puth(a0);
							uart_puts("is not its child )");
							uart_putc('\n');

							(get_runqueue()->curr_process)->tf->u_context[10] = (unsigned long)-1;
							break;
						}

						target->capability_root[idx] &= (~(15UL << offset));
						(get_runqueue()->curr_process)->tf->u_context[10] = (unsigned long)1;
					} else {
						(get_runqueue()->curr_process)->tf->u_context[10] = (unsigned long)-1;
					}

					break;
				}

			case 12:
				(get_runqueue()->curr_process)->tf->u_context[10] = alive_thread_counter;
				break;

			case 13:{
					unsigned long phys_addr = uvm_translate((get_runqueue()->curr_process)->satp, a0);

					if (phys_addr == 0){
						uart_puts("\n [RexOS] Process ");
						uart_puth((get_runqueue()->curr_process)->pid);
						uart_puts(" [");
						uart_puts((char*)(get_runqueue()->curr_process)->name);
						uart_puts("] ");
						uart_puts(" terminated (Segmentation Fault - Bad Syscall Pointer)\n");
						exit_process(-1);
						break;
					}

					unsigned long idx = 0;
					unsigned long target_idx = a1;

					if (target_idx <= 0) {
						uart_puts("\n [RexOS] Process ");
						uart_puth((get_runqueue()->curr_process)->pid);
						uart_puts(" [");
						uart_puts((char*)(get_runqueue()->curr_process)->name);
						uart_puts("] ");
						uart_puts(" ( Count of Processes to be listed must be positive )\n");
						(get_runqueue()->curr_process)->tf->u_context[10] = -1;
						break;	
					}
					
					struct process_info* pif = (struct process_info*) phys_addr;

					struct process* ptr = 0;

					for (int i = 0; i < 32; i++){

						if ((get_runqueue()->lookup_bitmap) & (1UL << i)){
							struct process* active_process_list_head = (get_runqueue()->active_process_circles)[i];
							ptr = active_process_list_head;

							do {
								pif->pid = ptr->pid;
								pif->parent_pid = ptr->parent_pid;
								pif->state = ptr->state;
								pif->priority = ptr->priority;
								pif->cpu_time = ptr->cpu_time;
								for (int i = 0; i < 24; i++) pif->name[i] = ptr->name[i];
								ptr = ptr->next;
								pif = (struct process_info*)((unsigned long)pif + (sizeof(struct process_info)));
								idx++;
							} while (ptr != active_process_list_head && idx < target_idx);
						}
					}

					ptr = blocked_process_list_head;

					while (ptr != 0 && idx < target_idx){
						pif->pid = ptr->pid;
						pif->parent_pid = ptr->parent_pid;
						pif->state = ptr->state;
						pif->priority = ptr->priority;
						pif->cpu_time = ptr->cpu_time;
						for (int i = 0; i < 24; i++) pif->name[i] = ptr->name[i];
						ptr = ptr->next;
						pif = (struct process_info*)((unsigned long)pif + (sizeof(struct process_info)));
						idx++;
					}

					ptr = blocked_ipc_process_list_head;

					while (ptr != 0 && idx < target_idx){
						pif->pid = ptr->pid;
						pif->parent_pid = ptr->parent_pid;
						pif->state = ptr->state;
						pif->priority = ptr->priority;
						pif->cpu_time = ptr->cpu_time;
						for (int i = 0; i < 24; i++) pif->name[i] = ptr->name[i];
						ptr = ptr->next;
						pif = (struct process_info*)((unsigned long)pif + (sizeof(struct process_info)));
						idx++;
					}
					
					ptr = sleeping_process_list_head;

					while (ptr != 0 && idx < target_idx){
						pif->pid = ptr->pid;
						pif->parent_pid = ptr->parent_pid;
						pif->state = ptr->state;
						pif->priority = ptr->priority;
						pif->cpu_time = ptr->cpu_time;
						for (int i = 0; i < 24; i++) pif->name[i] = ptr->name[i];
						ptr = ptr->next;
						pif = (struct process_info*)((unsigned long)pif + (sizeof(struct process_info)));
						idx++;
					}

					ptr = blocked_ipc_send_process_list_head;

					while (ptr != 0 && idx < target_idx){
						pif->pid = ptr->pid;
						pif->parent_pid = ptr->parent_pid;
						pif->state = ptr->state;
						pif->priority = ptr->priority;
						pif->cpu_time = ptr->cpu_time;
						for (int i = 0; i < 24; i++) pif->name[i] = ptr->name[i];
						ptr = ptr->next;
						pif = (struct process_info*)((unsigned long)pif + (sizeof(struct process_info)));
						idx++;
					}

					ptr = zombie_process_list_head;

					while (ptr != 0 && idx < target_idx){
						pif->pid = ptr->pid;
						pif->parent_pid = ptr->parent_pid;
						pif->state = ptr->state;
						pif->priority = ptr->priority;
						pif->cpu_time = ptr->cpu_time;
						for (int i = 0; i < 24; i++) pif->name[i] = ptr->name[i];
						ptr = ptr->next;
						pif = (struct process_info*)((unsigned long)pif + (sizeof(struct process_info)));
						idx++;
					}

					(get_runqueue()->curr_process)->tf->u_context[10] = idx;
					break;
				}
				
			case 14:
				
				switch(a1){

				case 1:{

					int present = 0;

					for (int i = 0; i < (get_runqueue()->curr_process)->active_grant; i++){
						if ((get_runqueue()->curr_process)->grant_whitelist[i] == a0){
							present = 1;
							break;
						}
					}

					if (present){
						(get_runqueue()->curr_process)->tf->u_context[10] = 1;
						break;
					} else {
						if ((get_runqueue()->curr_process)->active_grant == 8){
							uart_puts("\n [RexOS] Process ");
							uart_puth((get_runqueue()->curr_process)->pid);
							uart_puts(" [");
							uart_puts((char*)(get_runqueue()->curr_process)->name);
							uart_puts("] ");
							uart_puts("(Whitelist if already full)");
							uart_putc('\n');

							(get_runqueue()->curr_process)->tf->u_context[10] = (unsigned long)-1;
							break;
	
						} else {
							(get_runqueue()->curr_process)->grant_whitelist[(get_runqueue()->curr_process)->active_grant] = a0;
							(get_runqueue()->curr_process)->active_grant++;
							(get_runqueue()->curr_process)->tf->u_context[10] = (unsigned long)1;
							break;
						}	
					}					
				       }

				case 0:{
					int present = 0;
					int idx = 0;

					for (int i = 0; i < (get_runqueue()->curr_process)->active_grant; i++){
						if ((get_runqueue()->curr_process)->grant_whitelist[i] == a0){
							present = 1;
							idx = i;
							break;
						}
					}

					if (!present){
						(get_runqueue()->curr_process)->tf->u_context[10] = 1;
						break;
					} else {
						(get_runqueue()->curr_process)->grant_whitelist[idx] = (get_runqueue()->curr_process)->grant_whitelist[(get_runqueue()->curr_process)->active_grant - 1];
						(get_runqueue()->curr_process)->grant_whitelist[(get_runqueue()->curr_process)->active_grant - 1] = 0;	
							(get_runqueue()->curr_process)->active_grant--;
							(get_runqueue()->curr_process)->tf->u_context[10] = (unsigned long)1;
							break;
						}	
					}

				default:
					uart_puts("\n [RexOS] Process ");
					uart_puth((get_runqueue()->curr_process)->pid);
					uart_puts(" [");
					uart_puts((char*)(get_runqueue()->curr_process)->name);
					uart_puts("] ");
					uart_puts("(Unrecognized syscall argument.)");
					uart_putc('\n');
					(get_runqueue()->curr_process)->tf->u_context[10] = (unsigned long)-1;
					break;
				}
				break;

			case 15:{
					unsigned long phys_addr = uvm_translate((get_runqueue()->curr_process)->satp, a1);

					if (phys_addr == 0){
						uart_puts("\n [RexOS] Process ");
						uart_puth((get_runqueue()->curr_process)->pid);
						uart_puts(" [");
						uart_puts((char*)(get_runqueue()->curr_process)->name);
						uart_puts("] ");
						uart_puts(" terminated (Segmentation Fault - Bad Syscall Pointer)\n");
						exit_process(-1);
						break;
					}
					
					struct process* target = 0;

					struct process* ptr = 0;
					
					for (int i = 0; i < 32; i++){

						if ((get_runqueue()->lookup_bitmap) & (1UL << i)){
							struct process* active_process_list_head = (get_runqueue()->active_process_circles)[i];
							ptr = active_process_list_head;

							do {
								if ( ptr->pid == a0 ){
									target = ptr;
								}
			
								ptr = ptr->next;
							} while (ptr != active_process_list_head && target == 0);
						}
					}

					ptr = blocked_process_list_head;

					while (ptr != 0 && target == 0){
						if ( ptr->pid == a0){
							target = ptr;
						}

						ptr = ptr->next;
					}

					ptr = blocked_ipc_process_list_head;

					while (ptr != 0 && target == 0){
						if ( ptr->pid == a0){
							target = ptr;
						}

						ptr = ptr->next;
					}

					ptr = blocked_ipc_send_process_list_head;

					while (ptr != 0 && target == 0){
						if ( ptr->pid == a0){
							target = ptr;
						}

						ptr = ptr->next;
					}
					
					ptr = sleeping_process_list_head;

					while (ptr != 0 && target == 0){
						if ( ptr->pid == a0){
							target = ptr;
						}

						ptr = ptr->next;
					}
					
					if (target != 0){
						unsigned int idx = target->pid / 16;
						unsigned int offset = ((target->pid % 16) * 4);

						unsigned long rights = (((get_runqueue()->curr_process)->capability_root[idx] >> offset) & 15UL );

						if ( !(rights & 4UL) && ((get_runqueue()->curr_process)->pid != target->pid)) {
							uart_puts("\n [RexOS] Process ");
							uart_puth((get_runqueue()->curr_process)->pid);
							uart_puts(" [");
							uart_puts((char*)(get_runqueue()->curr_process)->name);
							uart_puts("] ");
							uart_puts("(Don't have right to grant )");
							uart_putc('\n');

							(get_runqueue()->curr_process)->tf->u_context[10] = (unsigned long)-1;
							break;
						}
	
						uvm_map((unsigned long*)((target->satp & 0xFFFFFFFFFFF) << 12), a2, phys_addr, 4096, a3);
						_flush_tlb();
						(get_runqueue()->curr_process)->tf->u_context[10] = (unsigned long)1;

					} else {
						(get_runqueue()->curr_process)->tf->u_context[10] = (unsigned long)-1;
					}

					break;
				}

			case 16:{
					struct process* target = 0;
					
					struct process* ptr = 0;

					for (int i = 0; i < 32; i++){

						if ((get_runqueue()->lookup_bitmap) & (1UL << i)){
							struct process* active_process_list_head = (get_runqueue()->active_process_circles)[i];
							ptr = active_process_list_head;

							do {
								if ( ptr->pid == a0 ){
									target = ptr;
								}
			
								ptr = ptr->next;
							} while (ptr != active_process_list_head && target == 0);
						}
					}

					ptr = blocked_process_list_head;

					while (ptr != 0 && target == 0){
						if ( ptr->pid == a0){
							target = ptr;
						}

						ptr = ptr->next;
					}

					ptr = blocked_ipc_process_list_head;

					while (ptr != 0 && target == 0){
						if ( ptr->pid == a0){
							target = ptr;
						}

						ptr = ptr->next;
					}

					ptr = blocked_ipc_send_process_list_head;

					while (ptr != 0 && target == 0){
						if ( ptr->pid == a0){
							target = ptr;
						}

						ptr = ptr->next;
					}
					
					ptr = sleeping_process_list_head;

					while (ptr != 0 && target == 0){
						if ( ptr->pid == a0){
							target = ptr;
						}

						ptr = ptr->next;
					}
					
					if (target != 0){
						
						unsigned int idx = target->pid / 16;
						unsigned int offset = ((target->pid % 16) * 4);

						unsigned long rights = (((get_runqueue()->curr_process)->capability_root[idx] >> offset) & 15UL );

						if ( !(rights & 4UL) && ((get_runqueue()->curr_process)->pid != target->pid) ) {
							uart_puts("\n [RexOS] Process ");
							uart_puth((get_runqueue()->curr_process)->pid);
							uart_puts(" [");
							uart_puts((char*)(get_runqueue()->curr_process)->name);
							uart_puts("] ");
							uart_puts("(Don't have right to grant )");
							uart_putc('\n');

							(get_runqueue()->curr_process)->tf->u_context[10] = (unsigned long)-1;
							break;
						}


						if (uvm_unmap(target->satp, a1)){
							_flush_tlb();
							(get_runqueue()->curr_process)->tf->u_context[10] = (unsigned long)1;
						} else {
							(get_runqueue()->curr_process)->tf->u_context[10] = (unsigned long)-1;
						}

					} else {
						(get_runqueue()->curr_process)->tf->u_context[10] = (unsigned long)-1;
					}

					break;	
				}

			case 17:{
					if ((get_runqueue()->curr_process)->pid != 1) {
						uart_puts("\n [RexOS] Process ");
						uart_puth((get_runqueue()->curr_process)->pid);
						uart_puts(" [");
						uart_puts((char*)(get_runqueue()->curr_process)->name);
						uart_puts("] ");
						uart_puts("(Hardware Claim Denied: Requires PID 1)\n");

						(get_runqueue()->curr_process)->tf->u_context[10] = (unsigned long)-1;
						break;
					}

					if (a0 >= 64) {
						(get_runqueue()->curr_process)->tf->u_context[10] = (unsigned long)-1;
						break;
					}

					irq_registry[a0] = (get_runqueue()->curr_process)->pid;

					(get_runqueue()->curr_process)->tf->u_context[10] = (unsigned long)1;
					break;
				}

			case 18:
				if (uvm_translate((get_runqueue()->curr_process)->satp, a0)){
					(get_runqueue()->curr_process)->tf->u_context[10] = 1;
				}else{
					(get_runqueue()->curr_process)->tf->u_context[10] = 0;
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

	if ((get_runqueue()->curr_process)->tf != 0) _load_sscratch((unsigned long)(get_runqueue()->curr_process)->tf);

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
	dtb_parser_hart();

	struct hart_runqueue* core0_hart_runqueue = (struct hart_runqueue*)kvmalloc(sizeof(struct hart_runqueue));

	unsigned char* ptr = (unsigned char*)core0_hart_runqueue;
	
	for (int i = 0; i < sizeof(struct hart_runqueue) ; i++){
		*(ptr + i) = 0;
	}
	
	_load_tp((unsigned long)core0_hart_runqueue);
	
	scheduler_init();
	timer_init();
	plic_init();
	uart_ier_enable();
	_set_ssie();
	_set_seie();

	shell_ptr = spawn_process(_binary_user_shell_elf_start, "SHELL", 32, 16);
	struct process* worker = spawn_process(_binary_user_worker_elf_start, "WORKER", 32, 20);
	
	global_closet = (struct cpu_closet*)kvmalloc((hart * sizeof(struct cpu_closet)));

	for (unsigned long i = 1; i < hart; i++){
		global_closet[i]._mmode_stack = (unsigned long*)((char*)kalloc(0) + 4096);
		global_closet[i]._smode_stack = (unsigned long*)((char*)kalloc(0) + 4096);
		global_closet[i].flag = 0;
	}

	for (unsigned long i = 1; i < hart; i++){
		global_closet[i].flag = 1;
		*(volatile unsigned int*)(CLINT_BASE + (i * 4)) = 1;
	}

	oxomoco_loop();
}
