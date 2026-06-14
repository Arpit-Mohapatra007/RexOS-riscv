#ifndef UART_H
#define UART_H

void uart_init(void);
void uart_putc(unsigned char text);
void uart_puts(char* string);
void uart_putsn(char* string, unsigned int max_length);
void uart_puth(unsigned long code);
unsigned char uart_getc(void);

#endif
