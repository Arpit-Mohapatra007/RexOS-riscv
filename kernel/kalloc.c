#include "kalloc.h"
#include "dtb.h"
#include "main.h"

#define MAX_ORDER 12

extern char _kernel_end[];

struct page *page_array;
unsigned long *free_lists; 
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
	
	if ( free_pool_start >= pool_horizon ) kpanic(106,free_pool_start);

	page_array = (struct page*) metadata_start;
	free_lists = (unsigned long*) &page_array[total_pages];

	for (unsigned long i = 0; i < total_pages; i++) {
		page_array[i].order = 0;
		page_array[i].flag = 2;
		page_array[i].next_idx = 0xFFFFFFFF;
		page_array[i].prev_idx = 0xFFFFFFFF;
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

void* kalloc(unsigned int order){
	
	if ( order >= MAX_ORDER ) kpanic(100,(unsigned long)order);

	unsigned int curr_order = order;
	unsigned long idx = free_lists[order];
	
	while ( idx == 0xFFFFFFFF ){
		curr_order++;
		if ( curr_order >= MAX_ORDER ){
			kpanic(101,0);
		}
		idx = free_lists[curr_order];
	}
	
	free_lists[curr_order] = page_array[idx].next_idx;

	unsigned long curr_head = free_lists[curr_order];

	if ( curr_head != 0xFFFFFFFF ) page_array[curr_head].prev_idx = 0xFFFFFFFF;

	while ( curr_order > order ){
		curr_order--;
		unsigned long buddy_idx = idx + ( 1 << curr_order );

		page_array[buddy_idx].order = curr_order;
		page_array[buddy_idx].flag = 0;
		page_array[buddy_idx].next_idx = free_lists[curr_order];
		free_lists[curr_order] = buddy_idx;

		unsigned long prev_page = page_array[buddy_idx].next_idx;

		if ( prev_page != 0xFFFFFFFF ) page_array[prev_page].prev_idx = buddy_idx;

		page_array[buddy_idx].prev_idx = 0xFFFFFFFF;
	}

	page_array[idx].flag = 1;
	page_array[idx].order = order;
	page_array[idx].next_idx = 0xFFFFFFFF;
	page_array[idx].prev_idx = 0xFFFFFFFF;

	unsigned long phys_addr = ram.ram_base_addr + ( idx * 4096 );
	return (void*) phys_addr;
}

void kfree(void* phys_addr){
	
	if ( ( (unsigned long)phys_addr & 0xFFF ) != 0 ) kpanic(104, (unsigned long)phys_addr);

	unsigned long idx = ( (unsigned long)phys_addr - ram.ram_base_addr ) / 4096;
	unsigned long total_pages = ram.ram_total_size / 4096;

	if ( idx >= total_pages ) kpanic(102, (unsigned long)phys_addr);
	if ( page_array[idx].flag == 0 ) kpanic(103, (unsigned long)phys_addr);
	if ( page_array[idx].flag == 2 ) kpanic(105, (unsigned long)phys_addr);

	unsigned int order = page_array[idx].order;

	
	while (order < MAX_ORDER - 1){	
		unsigned long buddy_idx = idx ^ ( 1 << order );
		if ( buddy_idx >= total_pages ) break;
		if ( page_array[buddy_idx].flag != 0 ) break;
		if ( page_array[buddy_idx].order != order ) break;
		
		if ( page_array[buddy_idx].prev_idx == 0xFFFFFFFF ) {
			free_lists[order] = page_array[buddy_idx].next_idx;
		} 
		if ( page_array[buddy_idx].prev_idx != 0xFFFFFFFF ) {
			page_array[page_array[buddy_idx].prev_idx].next_idx = page_array[buddy_idx].next_idx;
		}
		if ( page_array[buddy_idx].next_idx != 0xFFFFFFFF ) {
			page_array[page_array[buddy_idx].next_idx].prev_idx = page_array[buddy_idx].prev_idx;
		}
		
		page_array[buddy_idx].flag = 2;
		page_array[buddy_idx].order = 0;
		page_array[buddy_idx].prev_idx = 0xFFFFFFFF;
		page_array[buddy_idx].next_idx = 0xFFFFFFFF;
		
		idx = idx & buddy_idx;
		order++;
	}

	page_array[idx].flag = 0;
	page_array[idx].order = order;
	page_array[idx].next_idx = free_lists[order];
	
	unsigned long prev_page = page_array[idx].next_idx;

	if ( prev_page != 0xFFFFFFFF ) page_array[prev_page].prev_idx = idx;

	free_lists[order] = idx;
	page_array[idx].prev_idx = 0xFFFFFFFF;

	return;
}
