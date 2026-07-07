#include "memlayout.h"
#include "dtb.h"

volatile unsigned char* get_uart_reg(unsigned int offset) {
	return (volatile unsigned char*)((unsigned long)(UART0_BASE + offset));
}

volatile unsigned int* get_plic_reg(unsigned long offset){
	return (volatile unsigned int*)((unsigned long)(PLIC_BASE + offset));
}

void uart_init(void) {
	*(get_uart_reg(1)) = 0x00; // Disabled Interrupt
	*(get_uart_reg(3)) = 0x80; // Bit 7 = 1: DLAB == 1
	*(get_uart_reg(0)) = 0x03; // Setting Communication Speed [Lower Bytes]
	*(get_uart_reg(1)) = 0x00; // [Higher Bytes] 
	*(get_uart_reg(3)) = 0x03; // Bit 7 = 0: DLAB == 0 -> to lock speed settings
				 // Bit 3 = 0: Paritiy bit disabled
				 // Bit 2 = 0: Number of Stop Bits = 1
				 // Bit 1, Bit 0 = 1, 1:  8 data bits
				 // Ref: https://docs.freebsd.org/en/articles/serial-uart/#_82501645016550_registers
	*(get_uart_reg(2)) = 0x07; // Bit 2 = 1: Transmit FIFO Reset
				 // Bit 1 = 1: Receiver FIFO Reset
				 // Bit 0 = 1: Transmit-Receive FIFOs are enabled

	return;	
}

void uart_putc(unsigned char text) {
	//Polling loop
	while( ( *(get_uart_reg(5)) & 0x20 ) == 0 ); // Bit 5 : Transmitter FIFO is empty if 1 else when 0
	*(get_uart_reg(0)) = text;
	return;
}

void uart_puts(char* string) {
	while ( *string != '\0' ) {
		uart_putc(*string);
		string++;
	}
	return;
}

void uart_putsn(char* string, unsigned int max_length) {
	unsigned int counter = 0;
	while ( *string != '\0' && counter < max_length) {
		uart_putc(*string); string++; counter++;
	}
	return;
}

void uart_puth(unsigned long code) {
	for (int i = 0; i < 16; i++) {
		int nibble = ( ( code >> ( 60 - ( i * 4 ) ) ) & 0xF );
		if ( nibble <= 9 ) {
			uart_putc((char)(nibble + '0'));
		} else {
			uart_putc((char)(nibble - 10 + 'A'));
		}
	}
	return;
}

unsigned char uart_getc(void){ 
	//Bit 0 :  data is waiting in Receive FIFO if 1 else when 0
	if ( ( *(get_uart_reg(5)) & 0x01 ) == 1 ) return *(get_uart_reg(0));

	return 0x00;
}

void plic_init(void){
	*(get_plic_reg(0x28)) = 1;  
	*(get_plic_reg((0x2000 + (context_idx * 0x80)))) = (1 << 10);
	*(get_plic_reg((0x200000 + (context_idx * 0x1000)))) = 0;	
	return;
}

void uart_ier_enable(void){
	*(get_uart_reg(1)) = 0x1;

	return;
}

unsigned int plic_claim(void){
	return *(get_plic_reg((0x200004 + (context_idx * 0x1000))));
}

void plic_complete(unsigned int irq){
	*(get_plic_reg((0x200004 + (context_idx * 0x1000)))) = irq;
	return;
}
