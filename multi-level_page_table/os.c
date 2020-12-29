
#define _GNU_SOURCE

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <err.h>
#include <sys/mman.h>

#include "os.h"

/* 2^20 pages ought to be enough for anybody */
#define NPAGES	(1024*1024)

static void* pages[NPAGES];

uint64_t alloc_page_frame(void)
{
	static uint64_t nalloc;
	uint64_t ppn;
	void* va;

	if (nalloc == NPAGES)
		errx(1, "out of physical memory");

	/* OS memory management isn't really this simple */
	ppn = nalloc;
	nalloc++;

	va = mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	if (va == MAP_FAILED)
		err(1, "mmap failed");

	pages[ppn] = va;	
	return ppn;
}
uint64_t get_mask_val(uint64_t vpn,int i)
{
     vpn=vpn>>12;
     return ((vpn>>(i*9))&511);
}
void map(uint64_t pt, uint64_t vpn, uint64_t ppn)
{
    int i;
    uint64_t* ptr;
    uint64_t mask=0;
    vpn=vpn<<12;
    ptr=phys_to_virt(pt<<12);
    for(i=0; i<4; i++)
    {   
        mask=get_mask_val(vpn, i);
        if ((*(ptr+mask)&1)==0)
            *(ptr+mask)=((alloc_page_frame()<<12)|1);
        ptr=phys_to_virt(*(ptr+mask));
    }
    mask=get_mask_val(vpn,4);
    *(ptr+mask)=(ppn<<12|1);         
}
void no_map(uint64_t pt, uint64_t vpn)
{
    int i;
    uint64_t *ptr;
    uint64_t mask=0;
    vpn=vpn<<12;
    ptr=phys_to_virt(pt<<12);
    for(i=0; i<4; i++)
    {
        mask=get_mask_val(vpn, i);
        if ((*(ptr+mask)&1)==0)
            return;
        ptr=phys_to_virt(*(ptr+mask));
    }
    mask=get_mask_val(vpn,4);
    *(ptr+mask)=(*(ptr+mask)&0xfffffffffffffffe);   
}

void page_table_update(uint64_t pt, uint64_t vpn, uint64_t ppn)
{
    if (ppn==NO_MAPPING)
        no_map(pt,vpn);
    else map(pt,vpn,ppn);
}
uint64_t page_table_query(uint64_t pt, uint64_t vpn)
{
    int i;
    uint64_t *ptr;
    uint64_t mask=0;
    vpn=vpn<<12;
    ptr=phys_to_virt(pt<<12);
    for(i=0; i<4;i++)
    {
        mask=get_mask_val(vpn, i);
        if ((*(ptr+mask)&1)==0)      
            return NO_MAPPING;
        ptr=phys_to_virt(*(ptr+mask));
    }
    mask=get_mask_val(vpn,4);
    if((*(ptr+mask)&1)==0)
        return NO_MAPPING;
    else return *(ptr+mask)>>12;
}

void* phys_to_virt(uint64_t phys_addr)
{
	uint64_t ppn = phys_addr >> 12;
	uint64_t off = phys_addr & 0xfff;
	void* va = NULL;

	if (ppn < NPAGES)
		va = pages[ppn] + off;

	return va;
}

int main(int argc, char **argv)
{
	uint64_t pt = alloc_page_frame();
	assert(page_table_query(pt, 0xcafe) == NO_MAPPING);
	page_table_update(pt, 0xcafe, 0xf00d);
	assert(page_table_query(pt, 0xcafe) == 0xf00d);
	page_table_update(pt, 0xcafe, NO_MAPPING);
	assert(page_table_query(pt, 0xcafe) == NO_MAPPING);

	return 0;
}
