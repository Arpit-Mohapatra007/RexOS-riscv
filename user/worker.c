#include "util.h"

void main(void){
	unsigned long pid = getpid();

	while(1){
		puts("[MY PID is:]");
		puth(pid);
		puts("\n");

		// sleep(1000);

		// puts("Hello !! After 1000 ticks");

		// yield();

		// puts("Hello After Yield !!");
		
		struct ipc_msg buff;

		buff.type = 3;
		buff.data[1] = 0xDEADBEEF;

		unsigned long status = send_msg(1,&buff);
		
		if (status == 1) puts("[WORKER] Payload Delivered !\n");

		exit(1);

		puts("Forbidden Message !!");
	}
}
