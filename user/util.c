extern void _sys_write(unsigned char c);
extern char _sys_read(void);

void puts(char* string){
	unsigned long i = 0;
	while (string[i] != '\0'){
		_sys_write(string[i]);
		i++;
	}
	return;
}

void putc(char c){
	_sys_write(c);
	return;
}

int strcmp(char* s1,char* s2) {
	int idx = 0;
	while(1){
		if ( s1[idx] != s2[idx] ) return 0;
		if ( s1[idx] == '\0' ) return 1;
		idx++;
	}
}

char readc(void){
	return _sys_read();
}
