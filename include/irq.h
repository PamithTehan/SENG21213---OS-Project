#ifndef IRQ_H
#define IRQ_H

#include <stdint.h>

static inline uint32_t irq_save(void) {
    uint32_t flags;
    __asm__ volatile (
        "pushfl\n\t"
        "pop %0\n\t"
        "cli"
        : "=r"(flags)
        :
        : "memory"
    );
    return flags;
}

static inline void irq_restore(uint32_t flags) {
    __asm__ volatile (
        "push %0\n\t"
        "popfl"
        :
        : "r"(flags)
        : "memory", "cc"
    );
}

void idt_init(void);
void init_timer(uint32_t frequency);
void timer_handler(void);

#endif