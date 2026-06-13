int kstrcmp(char* s1,char* s2) {
	int idx = 0;
	while(1){
		if ( s1[idx] != s2[idx] ) return 0;
		if ( s1[idx] == '\0' ) return 1;
		idx++;
	}
}

unsigned int kstrlen(char* s) {
	unsigned int len = 0;
	while( s[len] != '\0' ){
		len++;
	}
	return len;
}

int kstr_has_prefix(char* prefix, char* str) {
	int idx = 0;
	while(1){
		if( prefix[idx] == '\0' ) return 1;
		if( str[idx] == '\0' ) return 0;
		if( prefix[idx] != str[idx] ) return 0;
		idx++;
	}
}
