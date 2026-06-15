#include "kalloc.h"
#include "main.h"

extern char _text_start[];
extern char _text_end[];
extern char _rodata_start[];
extern char _rodata_end[];
extern char _data_start[];
extern char _data_end[];
extern char _bss_start[];
extern char _bss_end[];

unsigned long* root_table;

void _load_satp(void);

void kvm_map(unsigned long* root_table,unsigned long vir_addr,unsigned long phys_addr, unsigned long size, unsigned int perm){
	unsigned long pa_vir_addr = ( vir_addr & ( ~0xFFF ) );
	unsigned long pa_phys_addr = ( phys_addr & ( ~0xFFF ) );
	size = ( ( size + 0xFFF ) & ( ~0xFFF ) );
	
	while ( size > 0 ){
		unsigned short vpn_2 = ( pa_vir_addr >> 30 ) & ( 0x1FF );
		unsigned short vpn_1 = ( pa_vir_addr >> 21 ) & ( 0x1FF );
		unsigned short vpn_0 = ( pa_vir_addr >> 12 ) & ( 0x1FF );
		
		unsigned long pt_lv1 = root_table[vpn_2];

		if ( !( pt_lv1 & ( 0x1 ) ) ) {
			unsigned long* new_page_lv1 = (unsigned long*)kalloc(0);
			for (int i = 0; i < 512;i++){
				new_page_lv1[i] = 0;
			}
			unsigned long ppn = ((unsigned long)new_page_lv1) >> 12;
			unsigned long pte = ( ( ppn << 10 ) | 0x1 );
			root_table[vpn_2] = pte;
		}

		unsigned long* lv1_table = (unsigned long*)( ( root_table[vpn_2] >> 10 ) << 12 );
		unsigned long pt_lv0 = lv1_table[vpn_1];

		if ( !(pt_lv0 & ( 0x1 ) ) ) {
			unsigned long* new_page_lv0 = (unsigned long*)kalloc(0);
			for (int i = 0; i < 512;i++){
				new_page_lv0[i] = 0;
			}
			unsigned long ppn = ((unsigned long)new_page_lv0) >> 12;
			unsigned long pte = ( ( ppn << 10 ) | 0x1 );
			lv1_table[vpn_1] = pte;	
		}

		unsigned long* lv0_table = (unsigned long*)( ( lv1_table[vpn_1] >> 10 ) << 12 );
		unsigned long f_entry = lv0_table[vpn_0];

		if ( f_entry & ( 0x1 ) ) kpanic(107,pa_phys_addr);

		unsigned long ppn = ( ( pa_phys_addr >> 12 ) << 10 );
		lv0_table[vpn_0] = ( ppn | perm | 0x1 );

		pa_vir_addr += 4096;
		pa_phys_addr += 4096;
		size -= 4096;
	}
	return;
}

void kvm_init(void){
	root_table = (unsigned long*)kalloc(0);
	
	for (int i = 0; i < 512; i++){
		root_table[i] = 0;
	}

	// uart
	kvm_map(root_table,0x10000000,0x10000000,4096,6);

	//.text
	kvm_map(root_table,(unsigned long)_text_start,(unsigned long)_text_start,(unsigned long)_text_end - (unsigned long)_text_start,10);

	//.rodata
	kvm_map(root_table,(unsigned long)_rodata_start,(unsigned long)_rodata_start,(unsigned long)_rodata_end - (unsigned long)_rodata_start,2);

	//.data
	kvm_map(root_table,(unsigned long)_data_start,(unsigned long)_data_start,(unsigned long)_data_end - (unsigned long)_data_start,6);
	
	//.bss	
	kvm_map(root_table,(unsigned long)_bss_start,(unsigned long)_bss_start,(unsigned long)_bss_end - (unsigned long)_bss_start,6);
	
	_load_satp();

	return;
}
