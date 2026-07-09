#include "util.h"

void main(void){
	unsigned long pid = getpid();

	while(1){
		puts("[MY PID is:]");
		puth(pid);
		puts("\n");

		sleep(1000);

		puts("Hello !! After 1000 ticks");

		yield();

		puts("Hello After Yield !!");

		exit(1);

		puts("Forbidden Message !!");
	}
}
