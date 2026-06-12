#include "memlayout.h"

volatile unsigned char* update_uart(unsigned int offset) {
	return (volatile unsigned char*)((unsigned long)(UART0_BASE + offset));
}

void uart_init(void) {
	*(update_uart(1)) = 0x00; // Disabled Interrupt
	*(update_uart(3)) = 0x80; // Bit 7 = 1: DLAB == 1
	*(update_uart(0)) = 0x03; // Setting Communication Speed [Lower Bytes]
	*(update_uart(1)) = 0x00; // [Higher Bytes] 
	*(update_uart(3)) = 0x03; // Bit 7 = 0: DLAB == 0 -> to lock speed settings
				 // Bit 3 = 0: Paritiy bit disabled
				 // Bit 2 = 0: Number of Stop Bits = 1
				 // Bit 1, Bit 0 = 1, 1:  8 data bits
				 // Ref: https://docs.freebsd.org/en/articles/serial-uart/#_82501645016550_registers
	*(update_uart(2)) = 0x07; // Bit 2 = 1: Transmit FIFO Reset
				 // Bit 1 = 1: Receiver FIFO Reset
				 // Bit 0 = 1: Transmit-Receive FIFOs are enabled

	return;	
}

void uart_putc(unsigned char text) {
	//Polling loop
	while( ( *(update_uart(5)) & 0x20 ) == 0 ); // Bit 5 : Transmitter FIFO is empty if 1 else when 0
	*(update_uart(0)) = text;
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

unsigned char uart_getc(void) {
	//Polling loop
	while( ( *(update_uart(5)) & 0x01 ) == 0 ); //Bit 0 :  Receive FIFO is empty if 1 else when 0
	return *(update_uart(0));
}
