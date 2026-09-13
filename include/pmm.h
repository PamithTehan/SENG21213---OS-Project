#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include <stddef.h>

#define PAGE_SIZE 4096  

void     pmm_init(uint32_t memsize);
uint32_t pmm_alloc_frame(void);
void     pmm_free_frame(uint32_t addr);

size_t   pmm_get_free_frame_count(void);
size_t   pmm_get_total_frame_count(void);

#endif