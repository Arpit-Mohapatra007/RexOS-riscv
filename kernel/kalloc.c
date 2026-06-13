#include "dtb.h"

#define MAX_ORDER 12

extern char _kernel_end[];

struct page {
	unsigned int order;
	unsigned int flag;
	unsigned long next_idx;
	unsigned long prev_idx;
};

void kalloc_init(void){
	dtb_parser();
	unsigned long metadata_start = ( ( (unsigned long)_kernel_end + 0xFFF ) & ( ~0xFFF ) );
	unsigned long total_pages = ram.ram_total_size / 4096;
	unsigned long metadata_blob_size = ( total_pages * sizeof(struct page) ) + ( MAX_ORDER * 16 );
	unsigned long metadata_end = metadata_start + metadata_blob_size;
	unsigned long max_block_size = ( 1UL << ( MAX_ORDER - 1 ) ) * 4096;
	unsigned long alignment_mask = max_block_size - 1;

	unsigned long free_pool_start = ( ( metadata_end + alignment_mask ) & ( ~alignment_mask ) );
	unsigned long pool_horizon = ram.ram_base_addr + ram.ram_total_size;
	
	struct page *page_array = (struct page*) metadata_start;
	unsigned int *free_lists = (unsigned int*) &page_array[total_pages];

	for (unsigned long i = 0; i < total_pages; i++) {
		page_array[i].order = 0;
		page_array[i].flag = 1;
	}

	unsigned long current_phys_addr = free_pool_start;
	for (int i = 0; i<MAX_ORDER; i++) {
		free_lists[i] = 0xFFFFFFFF;
	}

	while (current_phys_addr < pool_horizon) {

		unsigned long idx = ( current_phys_addr - ram.ram_base_addr ) / 4096;

		page_array[idx].flag = 0;
		page_array[idx].order = MAX_ORDER - 1;
		page_array[idx].next_idx = free_lists[MAX_ORDER-1];

		unsigned long prev_page = page_array[idx].next_idx;
		
		free_lists[MAX_ORDER-1] = idx;

		if ( ( prev_page ) != 0xFFFFFFFF ) page_array[prev_page].prev_idx = idx;
		
		page_array[idx].prev_idx = 0xFFFFFFFF;
		current_phys_addr += max_block_size;
	}
	return;
}
