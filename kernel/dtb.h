#ifndef DTB_H
#define DTB_H

struct ram_meta {
	unsigned long ram_base_addr;
	unsigned long ram_total_size;
};

extern struct ram_meta ram;
extern unsigned long timebase_freq;

unsigned int kswap_endian(unsigned int input_endian);
void dtb_parser_ram(void);
void dtb_parser_time(void);

#endif
