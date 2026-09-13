#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include "thread.h"

typedef struct {
    int count;
    thread_t *waitq;
} sem_t;

void sem_init(sem_t *s, int init_count);
void sem_wait(sem_t *s);
void sem_signal(sem_t *s);

#endif