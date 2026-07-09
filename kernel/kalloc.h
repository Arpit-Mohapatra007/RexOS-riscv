#ifndef KALLOC_H
#define KALLOC_H

struct page {
	unsigned int order;
	unsigned int flag;
	unsigned long next_idx;
	unsigned long prev_idx;
	void* alloc_caller;
};

extern struct page *page_array;

void kalloc_init(void);
void* kalloc(unsigned int order);
void kfree(void* page);

#endif
