#ifndef DTB_H
#define DTB_H

struct meta {
	unsigned long base_addr;
	unsigned long total_size;
};

extern struct meta ram;
extern struct meta uart;
extern struct meta clint;
extern struct meta plic;
extern unsigned long timebase_freq;
extern unsigned int context_idx;
extern unsigned long hart;

unsigned int kswap_endian(unsigned int input_endian);
void dtb_parser_ram(void);
void dtb_parser_mmio(void);
void dtb_parser_time(void);
void dtb_parser_context(void);
void dtb_parser_hart(void);

#endif
