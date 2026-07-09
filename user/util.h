#ifndef UTIL_H
#define UTIL_H

void puts(char* string);
void putc(char c);
void puth(unsigned long code); 
int strcmp(char* s1,char* s2);
char readc(void);
unsigned long getpid(void);
void yield(void);
void sleep(unsigned int ticks);
void exit (unsigned long code);

#endif
