
#include <stdint.h>

#define NO_MAPPING	(~0ULL)

uint64_t alloc_page_frame(void);
void* phys_to_virt(uint64_t phys_addr);

uint64_t get_mask_val(uint64_t vpn,int i);
void map(uint64_t pt, uint64_t vpn, uint64_t ppn);
void no_map(uint64_t pt, uint64_t vpn);

void page_table_update(uint64_t pt, uint64_t vpn, uint64_t ppn);
uint64_t page_table_query(uint64_t pt, uint64_t vpn);


