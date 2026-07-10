#include "kalloc.h"
#include "main.h"
#include "dtb.h"
#include "memlayout.h"

extern char _text_start[];
extern char _text_end[];
extern char _rodata_start[];
extern char _rodata_end[];
extern char _data_start[];
extern char _data_end[];
extern char _bss_start[];
extern char _bss_end[];
extern char _user_trampoline[];

#define NUM_KVMALLOC_CACHES 8

unsigned long* root_table;
void _load_satp(unsigned long table_root_addr);
extern void _flush_tlb(void);

struct slab{
	struct kvmem_cache* parent_cache;
	unsigned long active_obj_count;
	struct slab* prev;
	struct slab* next;
	unsigned long* free_slot;
	unsigned long virt_start;
	void* alloc_caller;
	void* phys_start;
};
struct kvmem_cache{
	unsigned int obj_size;
	unsigned int aligned_size;
	unsigned int flag;
	char cache_name[32];
	struct slab* list_full;
	struct slab* list_partial;
	struct slab* list_empty;
	struct kvmem_cache* next_cache;
};

void kvm_map(unsigned long* root_table,unsigned long vir_addr,unsigned long phys_addr, unsigned long size, unsigned int perm){
	unsigned long pa_vir_addr = ( vir_addr & ( ~0xFFF ) );
	unsigned long pa_phys_addr = ( phys_addr & ( ~0xFFF ) );
	size = ( ( size + 0xFFF ) & ( ~0xFFF ) );
	
	while ( size > 0 ){
		unsigned short vpn_2 = ( pa_vir_addr >> 30 ) & ( 0x1FF );
		
		if (size >= 0x40000000 && ((pa_vir_addr & 0x3FFFFFFF) == 0) && ((pa_phys_addr & 0x3FFFFFFF) == 0)){	
			unsigned long f_entry = root_table[vpn_2];

			if ( f_entry & ( 0x1 ) ) kpanic(107,pa_phys_addr);

			unsigned long ppn = ( ( pa_phys_addr >> 12 ) << 10 );

			unsigned long attributes = 0x40;
	
			if (perm & 4) attributes |= 0x80;
	
			root_table[vpn_2] = ( ppn | attributes | perm | 0x1 );
		
			pa_vir_addr += 0x40000000;
			pa_phys_addr += 0x40000000;
			size -= 0x40000000;
			continue;
		}

		unsigned short vpn_1 = ( pa_vir_addr >> 21 ) & ( 0x1FF );

		if (size >= 0x200000 && ((pa_vir_addr & 0x1FFFFF) == 0) && ((pa_phys_addr & 0x1FFFFF) == 0 )){
			unsigned long pt_lv1 = root_table[vpn_2];

			if ( !( pt_lv1 & ( 0x1 ) ) ) {
				unsigned long* new_page_lv1 = (unsigned long*)kalloc(0);
				
				if (new_page_lv1 == 0) kpanic(110,vir_addr);

				for (int i = 0; i < 512;i++){
					new_page_lv1[i] = 0;
				}
				unsigned long ppn = ((unsigned long)new_page_lv1) >> 12;
				unsigned long pte = ( ( ppn << 10 ) | 0x1 );
				root_table[vpn_2] = pte;
			}

			unsigned long* lv1_table = (unsigned long*)( ( root_table[vpn_2] >> 10 ) << 12 );
			unsigned long f_entry = lv1_table[vpn_1];

			if ( f_entry & ( 0x1 ) ) kpanic(107,pa_phys_addr);

			unsigned long ppn = ( ( pa_phys_addr >> 12 ) << 10 );

			unsigned long attributes = 0x40;
	
			if (perm & 4) attributes |= 0x80;
	
			lv1_table[vpn_1] = ( ppn | attributes | perm | 0x1 );
		
			pa_vir_addr += 0x200000;
			pa_phys_addr += 0x200000;
			size -= 0x200000;
			continue;
		}

		unsigned short vpn_0 = ( pa_vir_addr >> 12 ) & ( 0x1FF );
		
		unsigned long pt_lv1 = root_table[vpn_2];

		if ( !( pt_lv1 & ( 0x1 ) ) ) {
			unsigned long* new_page_lv1 = (unsigned long*)kalloc(0);
			
			if (new_page_lv1 == 0) kpanic(110,vir_addr);

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

			if (new_page_lv0 == 0) kpanic(111,vir_addr);

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

		unsigned long attributes = 0x40;

		if (perm & 4) attributes |= 0x80;

		lv0_table[vpn_0] = ( ppn | attributes | perm | 0x1 );

		pa_vir_addr += 4096;
		pa_phys_addr += 4096;
		size -= 4096;
	}

	_flush_tlb();
	return;
}

void kvm_init(void){
	root_table = (unsigned long*)kalloc(0);

	if (root_table == 0) kpanic(112,0);
	
	for (int i = 0; i < 512; i++){
		root_table[i] = 0;
	}

	// uart
	kvm_map(root_table,UART0_BASE,UART0_BASE,4096,6);

	//clint
	kvm_map(root_table,CLINT_BASE,CLINT_BASE,0x10000,6);

	//plic
	kvm_map(root_table,PLIC_BASE,PLIC_BASE,0x400000,6);

	//.text
	kvm_map(root_table,(unsigned long)_text_start,(unsigned long)_text_start,(unsigned long)_text_end - (unsigned long)_text_start,10);

	//.rodata
	kvm_map(root_table,(unsigned long)_rodata_start,(unsigned long)_rodata_start,(unsigned long)_rodata_end - (unsigned long)_rodata_start,2);

	//.data
	kvm_map(root_table,(unsigned long)_data_start,(unsigned long)_data_start,(unsigned long)_data_end - (unsigned long)_data_start,6);
	
	//.bss	
	kvm_map(root_table,(unsigned long)_bss_start,(unsigned long)_bss_start,(unsigned long)_bss_end - (unsigned long)_bss_start,6);

	//heap
	unsigned long ram_end = ram.ram_base_addr + ram.ram_total_size;
	unsigned long heap_size = ram_end - (unsigned long)_bss_end;
	
	kvm_map(root_table,(unsigned long)_bss_end,(unsigned long)_bss_end,heap_size,6);

	//trampoline
	kvm_map(root_table, 0x3FFFFFF000, (unsigned long)_user_trampoline, 4096, 8);

	_load_satp((unsigned long)root_table);

	return;
}

struct kvmem_cache kvmalloc_caches[NUM_KVMALLOC_CACHES];

void kvmalloc_init(void){
	
	const char* kvmalloc_names[NUM_KVMALLOC_CACHES] = {
		"kvmalloc-16",
		"kvmalloc-32",
		"kvmalloc-64",
		"kvmalloc-128",
		"kvmalloc-256",
		"kvmalloc-512",
		"kvmalloc-1024",
		"kvmalloc-2048"
	};

	for ( int i = 0; i < NUM_KVMALLOC_CACHES; i++ ){
		kvmalloc_caches[i].obj_size = ( 16 * ( 1 << i ) );
		kvmalloc_caches[i].aligned_size = ( 16 * ( 1 << i ) );
		
		if ( kvmalloc_caches[i].obj_size > 512 ){
			kvmalloc_caches[i].flag = 1;
		} else {
			kvmalloc_caches[i].flag = 0;
		}

		int j = 0;

		while ( kvmalloc_names[i][j] != '\0' && j < 31 ){
			kvmalloc_caches[i].cache_name[j] = kvmalloc_names[i][j];
			j++;
		}

		kvmalloc_caches[i].cache_name[j] = '\0';
		kvmalloc_caches[i].list_full = 0;
		kvmalloc_caches[i].list_partial = 0;
		kvmalloc_caches[i].list_empty = 0;

		if ( i < NUM_KVMALLOC_CACHES - 1 ){
			kvmalloc_caches[i].next_cache = &kvmalloc_caches[i+1];
		} else {
			kvmalloc_caches[i].next_cache = 0;
		}
	}

	return;
}

void* kvmem_cache_alloc(struct kvmem_cache* cache, void* caller){
	
	unsigned long* slot;

	if ( cache->list_partial != 0 ){
		struct slab* curr_slab = cache->list_partial;
		
		slot = curr_slab->free_slot;
		curr_slab->active_obj_count++;
		
		if ( ((!cache->flag) && (curr_slab->active_obj_count == ( ( 4096 - sizeof(struct slab) ) / (cache->aligned_size) ))) || ((cache->flag) && (curr_slab->active_obj_count == (4096/(cache->aligned_size)))) ){
			cache->list_partial = curr_slab->next;

			if (cache->list_partial != 0) cache->list_partial->prev = 0;
			
			curr_slab->next = cache->list_full;
			curr_slab->prev = 0;

			if (cache->list_full != 0) cache->list_full->prev = curr_slab;

			cache->list_full = curr_slab;
		}
		curr_slab->free_slot = (unsigned long*)(*(curr_slab->free_slot));
	} else if ( cache->list_partial == 0 && cache->list_empty != 0 ){
		struct slab* curr_slab = cache->list_empty;

		slot = curr_slab->free_slot;
		curr_slab->active_obj_count = 1;
		cache->list_empty = curr_slab->next;
		curr_slab->next = cache->list_partial;
		
		if ( cache->list_empty != 0 ) cache->list_empty->prev = 0;

		cache->list_partial = curr_slab;
		curr_slab->prev = 0;

		curr_slab->free_slot = (unsigned long*)(*(curr_slab->free_slot));
	} else {
		struct slab* new_slab = (struct slab*) kalloc(0);

		if (new_slab == 0) kpanic(108,(unsigned long)cache);
		
		unsigned long first_obj = 0;
		unsigned long max_objs = 0;
		struct slab* header = 0;
		
		if (!cache->flag){
			first_obj = (unsigned long)new_slab + sizeof(struct slab);
			max_objs = ( ( 4096 - sizeof(struct slab) ) / (cache->aligned_size) );
			new_slab->parent_cache = cache;
		} else {
			first_obj = (unsigned long)new_slab;
			max_objs = 4096 / cache->aligned_size;
			header = (struct slab*)kvmem_cache_alloc(&kvmalloc_caches[2], caller);
			
			if (header == 0) kpanic(109,(unsigned long)cache);

			header->parent_cache = cache;
		}
		unsigned int i = 0;
		while ( i < max_objs - 1 ){
			unsigned long curr_addr = first_obj + (i * cache->aligned_size);
			unsigned long next_addr = first_obj + ((i+1)*cache->aligned_size);
			*(unsigned long*) curr_addr = next_addr;
			i++;
		}
		
		*(unsigned long*)(first_obj + (i * cache->aligned_size)) = 0;

		slot = (unsigned long*)first_obj;
		if (!cache->flag){
			new_slab->free_slot = (unsigned long*)(first_obj + cache->aligned_size);
			new_slab->active_obj_count = 1;
			new_slab->next = 0;
			new_slab->prev = 0;
			new_slab->virt_start = (unsigned long)new_slab;
			new_slab->phys_start = (void*)new_slab;
			new_slab->alloc_caller = caller;
			cache->list_partial = new_slab;
		} else {
			header->free_slot = (unsigned long*)(first_obj + cache->aligned_size);
			header->active_obj_count = 1;
			header->next = 0;
			header->prev = 0;
			header->virt_start = (unsigned long)new_slab;
			header->phys_start = (void*)new_slab;
			header->alloc_caller = caller;
			cache->list_partial = header;
		}
	}
	return (void*)slot;
}

void* kvmalloc(unsigned long size){
	void* caller = __builtin_return_address(0);
	unsigned long* slot;

	if (!size){
		slot = 0;
	} else if (size <= 2048){
		unsigned int i = 0;
		
		while (kvmalloc_caches[i].aligned_size < size) i++;
		
		slot = (unsigned long*)kvmem_cache_alloc(&kvmalloc_caches[i], caller);
	} else{
		unsigned int order = 0;

		while ( (4096UL << order) < size ) order++;
		
		slot = (unsigned long*)kalloc(order);
	}

	return (void*)slot;
}

void kvmfree(void* slot){
	if ( !slot) return;
	
	unsigned long base_addr = ( (unsigned long)slot & (~0xFFF) );

	struct slab* curr_header = (struct slab*)base_addr;
	struct kvmem_cache* curr_parent_cache = curr_header->parent_cache;

	for(int i = 0;i<6;i++){
		if (&kvmalloc_caches[i] == curr_parent_cache){
			//on-slab
			*(unsigned long*) slot = (unsigned long)curr_header->free_slot;
			curr_header->free_slot = (unsigned long*) slot;
			unsigned int max_objs = (4096 - sizeof(struct slab))/(curr_parent_cache->aligned_size);
			curr_header->active_obj_count--;

			if (curr_header->active_obj_count == (max_objs - 1)){
				if (curr_header->prev != 0){
					curr_header->prev->next = curr_header->next;
				}else{
					curr_parent_cache->list_full = curr_header->next;
				}

				if (curr_header->next != 0 ) curr_header->next->prev = curr_header->prev;

				curr_header->next = curr_parent_cache->list_partial;
				curr_header->prev = 0;

				if (curr_header->next != 0) curr_header->next->prev = curr_header;

				curr_parent_cache->list_partial = curr_header;
			}else if (curr_header->active_obj_count == 0){
				if (curr_header->prev != 0){
					curr_header->prev->next = curr_header->next;
				}else{
					curr_parent_cache->list_partial = curr_header->next;
				}

				if (curr_header->next != 0 ) curr_header->next->prev = curr_header->prev;

				curr_header->next = curr_parent_cache->list_empty;
				curr_header->prev = 0;

				if (curr_header->next != 0) curr_header->next->prev = curr_header;

				curr_parent_cache->list_empty = curr_header;	
			}
						
			return;
		}
	}

	for(int i = 6;i<NUM_KVMALLOC_CACHES;i++){
		curr_header = kvmalloc_caches[i].list_full;
		curr_parent_cache = &kvmalloc_caches[i];
		while(curr_header){
			if (curr_header->virt_start == base_addr){
				//off-slab
				*(unsigned long*) slot = (unsigned long)curr_header->free_slot;
				curr_header->free_slot = (unsigned long*) slot;
				curr_header->active_obj_count--;
				if (curr_header->prev != 0){
					curr_header->prev->next = curr_header->next;
				}else{
					curr_parent_cache->list_full = curr_header->next;
				}

				if (curr_header->next != 0 ) curr_header->next->prev = curr_header->prev;

				curr_header->next = curr_parent_cache->list_partial;
				curr_header->prev = 0;

				if (curr_header->next != 0) curr_header->next->prev = curr_header;

				curr_parent_cache->list_partial = curr_header;
	
				return;
			}
			curr_header = curr_header->next;
		}

		curr_header = kvmalloc_caches[i].list_partial;
		while(curr_header){
			if (curr_header->virt_start == base_addr){
				//off-slab
				*(unsigned long*) slot = (unsigned long)curr_header->free_slot;
				curr_header->free_slot = (unsigned long*) slot;
				curr_header->active_obj_count--;
				if (curr_header->active_obj_count == 0){
					if (curr_header->prev != 0){
						curr_header->prev->next = curr_header->next;
					}else{
						curr_parent_cache->list_partial = curr_header->next;
					}

					if (curr_header->next != 0 ) curr_header->next->prev = curr_header->prev;

					curr_header->next = curr_parent_cache->list_empty;
					curr_header->prev = 0;

					if (curr_header->next != 0) curr_header->next->prev = curr_header;

					curr_parent_cache->list_empty = curr_header;	
				}
				return;
			}
			curr_header = curr_header->next;
		}
	}
	
	//free alloc
	kfree((void*)base_addr);	
	return;
}

void uvm_map(unsigned long* user_table,unsigned long vir_addr,unsigned long phys_addr, unsigned long size, unsigned int perm){
	unsigned long pa_vir_addr = ( vir_addr & ( ~0xFFF ) );
	unsigned long pa_phys_addr = ( phys_addr & ( ~0xFFF ) );
	size = ( ( size + 0xFFF ) & ( ~0xFFF ) );
	
	while ( size > 0 ){
		unsigned short vpn_2 = ( pa_vir_addr >> 30 ) & ( 0x1FF );
		
		if (size >= 0x40000000 && ((pa_vir_addr & 0x3FFFFFFF) == 0) && ((pa_phys_addr & 0x3FFFFFFF) == 0)){	
			unsigned long f_entry = user_table[vpn_2];

			if ( f_entry & ( 0x1 ) ) kpanic(107,pa_phys_addr);

			unsigned long ppn = ( ( pa_phys_addr >> 12 ) << 10 );

			unsigned long attributes = 0x40;
	
			if (perm & 4) attributes |= 0x80;
	
			user_table[vpn_2] = ( ppn | attributes | 0x10 | perm | 0x1 );
		
			pa_vir_addr += 0x40000000;
			pa_phys_addr += 0x40000000;
			size -= 0x40000000;
			continue;
		}

		unsigned short vpn_1 = ( pa_vir_addr >> 21 ) & ( 0x1FF );

		if (size >= 0x200000 && ((pa_vir_addr & 0x1FFFFF) == 0) && ((pa_phys_addr & 0x1FFFFF) == 0 )){
			unsigned long pt_lv1 = user_table[vpn_2];

			if ( !( pt_lv1 & ( 0x1 ) ) ) {
				unsigned long* new_page_lv1 = (unsigned long*)kalloc(0);
				
				if (new_page_lv1 == 0) kpanic(110,vir_addr);

				for (int i = 0; i < 512;i++){
					new_page_lv1[i] = 0;
				}
				unsigned long ppn = ((unsigned long)new_page_lv1) >> 12;
				unsigned long pte = ( ( ppn << 10 ) | 0x1 );
				user_table[vpn_2] = pte;
			}

			unsigned long* lv1_table = (unsigned long*)( ( user_table[vpn_2] >> 10 ) << 12 );
			unsigned long f_entry = lv1_table[vpn_1];

			if ( f_entry & ( 0x1 ) ) kpanic(107,pa_phys_addr);

			unsigned long ppn = ( ( pa_phys_addr >> 12 ) << 10 );

			unsigned long attributes = 0x40;
	
			if (perm & 4) attributes |= 0x80;
	
			lv1_table[vpn_1] = ( ppn | attributes | 0x10 | perm | 0x1 );
		
			pa_vir_addr += 0x200000;
			pa_phys_addr += 0x200000;
			size -= 0x200000;
			continue;
		}

		unsigned short vpn_0 = ( pa_vir_addr >> 12 ) & ( 0x1FF );
		
		unsigned long pt_lv1 = user_table[vpn_2];

		if ( !( pt_lv1 & ( 0x1 ) ) ) {
			unsigned long* new_page_lv1 = (unsigned long*)kalloc(0);
			
			if (new_page_lv1 == 0) kpanic(110,vir_addr);

			for (int i = 0; i < 512;i++){
				new_page_lv1[i] = 0;
			}
			unsigned long ppn = ((unsigned long)new_page_lv1) >> 12;
			unsigned long pte = ( ( ppn << 10 ) | 0x1 );
			user_table[vpn_2] = pte;
		}

		unsigned long* lv1_table = (unsigned long*)( ( user_table[vpn_2] >> 10 ) << 12 );
		unsigned long pt_lv0 = lv1_table[vpn_1];

		if ( !(pt_lv0 & ( 0x1 ) ) ) {
			unsigned long* new_page_lv0 = (unsigned long*)kalloc(0);

			if (new_page_lv0 == 0) kpanic(111,vir_addr);

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

		unsigned long attributes = 0x40;

		if (perm & 4) attributes |= 0x80;

		lv0_table[vpn_0] = ( ppn | attributes | 0x10 | perm | 0x1 );

		pa_vir_addr += 4096;
		pa_phys_addr += 4096;
		size -= 4096;
	}
	_flush_tlb();
	return;
}

unsigned long* create_user_table(unsigned long* process_kstack){
	unsigned long* new_user_table = (unsigned long*)(kalloc(0));

	for (int i = 0; i < 512; i++){
		new_user_table[i] = 0;
	}
	
	kvm_map(new_user_table, 0x3FFFFFF000, (unsigned long)_user_trampoline, 4096, 8);

	kvm_map(new_user_table, 0x3FFFFFE000, (unsigned long)process_kstack, 4096, 6);

	return new_user_table;
}

void uvm_free(unsigned long user_satp){
	unsigned long* lv2_table = (unsigned long*)((user_satp & 0xFFFFFFFFFFF) << 12);

	for (int i = 0; i < 512; i++){
		if (lv2_table[i] & 0x1){

			if ( lv2_table[i] & 0xE ) {
				if ( lv2_table[i] & 0x10 ){ 
					unsigned long leaf_phys_addr = ((lv2_table[i] >> 10) << 12);
					unsigned long idx = ((leaf_phys_addr - ram.ram_base_addr) / 4096);

					if (page_array[idx].flag == 1) kfree((void*)leaf_phys_addr);

					lv2_table[i] = 0;
				}
				continue;
	
			} else {
				unsigned long* lv1_table = (unsigned long*)(( lv2_table[i] >> 10) << 12);
				for (int j = 0; j < 512; j++){
					if (lv1_table[j] & 0x1){

						if ( lv1_table[j] & 0xE ) {
							if ( lv1_table[j] & 0x10 ){
								unsigned long leaf_phys_addr = ((lv1_table[j] >> 10) << 12);
								unsigned long idx = ((leaf_phys_addr - ram.ram_base_addr) / 4096);

								if (page_array[idx].flag == 1) kfree((void*)leaf_phys_addr);

								lv1_table[j] = 0;
							}
							continue;
	
						} else {
	
							unsigned long* lv0_table = (unsigned long*)((lv1_table[j] >> 10) << 12);
							for (int k = 0; k < 512; k++){
								if (( lv0_table[k] & 0x10 ) && ( lv0_table[k] & 0x1 )){
									unsigned long leaf_phys_addr = ((lv0_table[k] >> 10) << 12);
									unsigned long idx = ((leaf_phys_addr - ram.ram_base_addr) / 4096);

									if (page_array[idx].flag == 1) kfree((void*)leaf_phys_addr);

									lv0_table[k] = 0;
								}
							}

							kfree((void*)lv0_table);
						}
					}
				}

				kfree((void*)lv1_table);
			}
		}
	}

	kfree((void*)lv2_table);

	return;
}

unsigned long uvm_translate(unsigned long user_satp, unsigned long vir_addr) {
	unsigned long* lv2_table = (unsigned long*)((user_satp & 0xFFFFFFFFFFF) << 12);

	unsigned long pte_lv2 = lv2_table[( (vir_addr >> 30) & 0x1FF )];

	if (!(pte_lv2 & 0x1)) return 0;

	unsigned long* lv1_table = (unsigned long*)((pte_lv2 >> 10) << 12);

	unsigned long pte_lv1 = lv1_table[(( vir_addr >> 21 ) & 0x1FF )];

	if (!(pte_lv1 & 0x1)) return 0;

	unsigned long* lv0_table = (unsigned long*)((pte_lv1 >> 10) << 12);

	unsigned long pte_lv0 = lv0_table[(( vir_addr >> 12 ) & 0x1FF )];

	if (!(pte_lv0 & 0x1)) return 0;

	unsigned long phys_addr = ((pte_lv0 >> 10) << 12 ) | (vir_addr & 0xFFF);

	return phys_addr;
 
}
