#include "util.h"

void main(void){
	unsigned long pid = getpid();
	struct ipc_msg buff;

	buff.type = 3;
	buff.data[1] = 0xDEADBEEF;

	while(1){
		// puts("[MY PID is:]");
		// puth(pid);
		// puts("\n");

		 
		// puts("Hello !! After 1000 ticks");

		// yield();

		// puts("Hello After Yield !!");
		

		unsigned long status = send_msg(1,&buff);
		
		if (status == 1) puts("[WORKER] Payload Delivered !\n");
		
		
		sleep(1000);

		// exit(1);

		// puts("Forbidden Message !!");
	}
}
