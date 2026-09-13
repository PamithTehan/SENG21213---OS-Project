#include "pmm.h"

#define MAX_MEM_BYTES   (16 * 1024 * 1024)             
#define TOTAL_FRAMES    (MAX_MEM_BYTES / PAGE_SIZE)     
#define BITMAP_SIZE     (TOTAL_FRAMES / 32)            


static uint32_t pmm_bitmap[BITMAP_SIZE];
static uint32_t total_memory_size = 0;
static uint32_t total_frames = 0;
static uint32_t used_frames = 0;


static inline void set_frame(uint32_t frame) {
    pmm_bitmap[frame / 32] |= (1U << (frame % 32));
}

static inline void clear_frame(uint32_t frame) {
    pmm_bitmap[frame / 32] &= ~(1U << (frame % 32));
}

static inline int test_frame(uint32_t frame) {
    return (pmm_bitmap[frame / 32] & (1U << (frame % 32))) != 0;
}

void pmm_init(uint32_t memsize) {

    total_memory_size = memsize;
    total_frames = memsize / PAGE_SIZE;

    if (total_frames > TOTAL_FRAMES) {
        total_frames = TOTAL_FRAMES;
    }

    for (uint32_t i = 0; i < BITMAP_SIZE; i++) {
        pmm_bitmap[i] = 0;
    }

    used_frames = 0;

    uint32_t reserved_frames = 0x100000 / PAGE_SIZE; 

    uint32_t kernel_end_frames = 0x200000 / PAGE_SIZE;
    if (kernel_end_frames > total_frames) {
        kernel_end_frames = reserved_frames;
    }

    for (uint32_t frame = 0; frame < kernel_end_frames; frame++) {
        set_frame(frame);
        used_frames++;
    }
}

uint32_t pmm_alloc_frame(void) {
    
    for (uint32_t i = 0; i < total_frames / 32; i++) {

        if (pmm_bitmap[i] != 0xFFFFFFFF) { 

            for (int j = 0; j < 32; j++) {

                uint32_t frame = i * 32 + j;
                
                if (frame >= total_frames) {
                    return 0;
                }

                if (!test_frame(frame)) {
                    set_frame(frame);
                    used_frames++;
                    return frame * PAGE_SIZE; 
                }
            }
        }
    }
    return 0; 
}

void pmm_free_frame(uint32_t addr) {

    uint32_t frame = addr / PAGE_SIZE;
    
    if (frame < total_frames && test_frame(frame)) {
        clear_frame(frame);
        if (used_frames > 0) {
            used_frames--;
        }
    }
}

size_t pmm_get_free_frame_count(void) {
    return total_frames - used_frames;
}

size_t pmm_get_total_frame_count(void) {
    return total_frames;
}