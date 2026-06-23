#include "string.h"
#include "dtb.h"

struct fdtb_header {
	unsigned int magic;
	unsigned int totalsize;
	unsigned int off_dt_struct;
	unsigned int off_dt_strings;
	unsigned int off_mem_rsvmap;
	unsigned int version;
	unsigned int last_comp_version;
	unsigned int boot_cpuid_phys;
	unsigned int size_dt_strings;
	unsigned int size_dt_struct;
};


extern void* _dtb_ptr;
struct ram_meta ram;
unsigned long timebase_freq = 0;

unsigned int kswap_endian(unsigned int input_endian){
	unsigned int output_endian = 0;
	for ( int i = 0; i < 4; i++ ){
		int byte = ( ( input_endian >> (24 - (8 * i))) & 0xFF );
		byte = ( byte << ( 8 * i ));
		output_endian = ( output_endian | byte );
	}
	return output_endian;
}

void dtb_parser_ram(void){
	struct fdtb_header *fdt;
	fdt = _dtb_ptr;
	if ( kswap_endian(fdt->magic) == 0xD00DFEED ){
		unsigned int* parser_ptr_struct = (unsigned int*)( (char*)_dtb_ptr + kswap_endian(fdt->off_dt_struct)	);
		unsigned char* parser_ptr_strings = (unsigned char*)( (char*)_dtb_ptr + kswap_endian(fdt->off_dt_strings) );
		
		int inside_memory = 0;
		unsigned long ram_base = 0;
		unsigned long ram_size = 0;

		unsigned int* end_dtb = (unsigned int*)( (char*)_dtb_ptr + kswap_endian(fdt->totalsize) );
		unsigned int* end_tk_ptr = parser_ptr_struct + ( kswap_endian(fdt->size_dt_struct)/4 );

		while(parser_ptr_struct < end_tk_ptr && parser_ptr_struct < end_dtb){
			unsigned int token = kswap_endian(*parser_ptr_struct);
			parser_ptr_struct++;
			switch(token){
				case 0x00000001:
					if( kstr_has_prefix( "memory",(char*)parser_ptr_struct ) ){
						inside_memory = 1;
					} else{
						inside_memory = 0;
					}
					parser_ptr_struct += ( ( ( kstrlen( (char*)parser_ptr_struct ) + 1 + 3 ) & ( ~3 ) )/4 );
					break;
				case 0x00000003: {
					unsigned int prop_len = kswap_endian(parser_ptr_struct[0]);
					unsigned int prop_nameoff = kswap_endian(parser_ptr_struct[1]);

					parser_ptr_struct += 2;

					if ( inside_memory && kstrcmp( "reg",(char*) ( parser_ptr_strings + prop_nameoff ) ) ){
						ram_base = kswap_endian( parser_ptr_struct[0] );
						ram_base = ( ram_base << 32 );
						ram_base = ram_base | kswap_endian( parser_ptr_struct[1] );
						ram.ram_base_addr = ram_base;

						ram_size = kswap_endian( parser_ptr_struct[2] );
						ram_size = ( ram_size << 32 );
						ram_size = ram_size | kswap_endian( parser_ptr_struct[3] );
						ram.ram_total_size = ram_size;
					}
					parser_ptr_struct += ( ( ( prop_len + 3 ) & ( ~3 ) )/4 );
					break;
				}
				default:
					break;
			}
		}
	}
	return;
}

void dtb_parser_time(void){
struct fdtb_header *fdt;
	fdt = _dtb_ptr;
	if ( kswap_endian(fdt->magic) == 0xD00DFEED ){
		unsigned int* parser_ptr_struct = (unsigned int*)( (char*)_dtb_ptr + kswap_endian(fdt->off_dt_struct)	);
		unsigned char* parser_ptr_strings = (unsigned char*)( (char*)_dtb_ptr + kswap_endian(fdt->off_dt_strings) );
		
		int cpus_depth = 0;

		unsigned int* end_dtb = (unsigned int*)( (char*)_dtb_ptr + kswap_endian(fdt->totalsize) );
		unsigned int* end_tk_ptr = parser_ptr_struct + ( kswap_endian(fdt->size_dt_struct)/4 );

		while(parser_ptr_struct < end_tk_ptr && parser_ptr_struct < end_dtb){
			unsigned int token = kswap_endian(*parser_ptr_struct);
			parser_ptr_struct++;
			switch(token){
				case 0x00000001:
					if( kstr_has_prefix( "cpus",(char*)parser_ptr_struct ) ){
						cpus_depth = 1;
					} else if (cpus_depth > 0){
						cpus_depth++;
					}

					parser_ptr_struct += ( ( ( kstrlen( (char*)parser_ptr_struct ) + 1 + 3 ) & ( ~3 ) )/4 );
					break;

				case 0x00000002:
					if (cpus_depth > 0) cpus_depth--;
					break;

				case 0x00000003: {
					unsigned int prop_len = kswap_endian(parser_ptr_struct[0]);
					unsigned int prop_nameoff = kswap_endian(parser_ptr_struct[1]);

					parser_ptr_struct += 2;

					if ( cpus_depth > 0 && kstrcmp( "timebase-frequency",(char*) ( parser_ptr_strings + prop_nameoff ) ) ){
						timebase_freq = kswap_endian( parser_ptr_struct[0] );
						
						if (prop_len == 8){
							timebase_freq = (timebase_freq << 32);
							timebase_freq = timebase_freq | kswap_endian(parser_ptr_struct[1]);
						}
					}

					parser_ptr_struct += ( ( ( prop_len + 3 ) & ( ~3 ) )/4 );
					break;
				}
				default:
					break;
			}
		}
	}	
	return;
}
