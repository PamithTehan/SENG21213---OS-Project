#ifndef IRQ_H
#define IRQ_H

#include <stdint.h>

void init_timer(uint32_t frequency);
void timer_handler(void);

#endif